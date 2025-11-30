#include "Rendering/InstancedRenderer.h"
#include "Rendering/PBRLightingRenderer.h"
#include "Scene/SceneRenderer.h"
#include "Pipeline/RenderPass.h"
#include "Utility/RenderData.h"
#include "Utility/AABB.h"
#include "Resources/Mesh.h"
#include "Resources/Material.h"
#include <glad/glad.h>
#include <spdlog/spdlog.h>
#include <cassert>
#include <algorithm>

InstancedRenderer::InstancedRenderer(PBRLightingRenderer* lighting)
    : m_MaxInstances(10000), m_TotalInstances(0), m_BatchActive(false), m_PBRLighting(lighting)
{
    assert(lighting && "PBRLightingRenderer cannot be null");
    assert(m_MaxInstances > 0 && "Max instances must be positive");
}

InstancedRenderer::~InstancedRenderer()
{
    Clear();
}

void InstancedRenderer::BeginInstanceBatch()
{
    if (m_BatchActive) {
        return;
    }
    
    m_BatchActive = true;
    m_TotalInstances = 0;
    
    // Clear existing instances but keep mesh data
    for (auto& pair : m_MeshInstances) {
        pair.second.instances.clear();
        pair.second.dirty = true;
    }
}

void InstancedRenderer::AddInstance(const std::string& meshId, const InstanceData& instance)
{
    assert(m_BatchActive && "Must call BeginInstanceBatch() first");
    assert(!meshId.empty() && "Mesh ID cannot be empty");
    assert(m_TotalInstances < m_MaxInstances && "Cannot exceed maximum instances");

    if (!m_BatchActive) {
        spdlog::error("Must call BeginInstanceBatch() first");
        return;
    }

    if (m_TotalInstances >= m_MaxInstances) {
        spdlog::warn("Exceeded maximum instances ({})", m_MaxInstances);
        return;
    }

    auto& meshInstances = m_MeshInstances[meshId];
    meshInstances.instances.push_back(instance);
    meshInstances.dirty = true;
    m_TotalInstances++;
}

void InstancedRenderer::EndInstanceBatch()
{
    if (!m_BatchActive) {
        spdlog::warn("No active instance batch");
        return;
    }
    
    m_BatchActive = false;
    
    // Update SSBOs for dirty meshes
    for (auto& pair : m_MeshInstances) {
        if (pair.second.dirty && !pair.second.instances.empty()) {
            UpdateInstanceSSBO(pair.first);
        }
    }
    
}

void InstancedRenderer::Clear()
{
    m_MeshInstances.clear();
    m_InstanceSSBOs.clear();
    m_TotalInstances = 0;
    m_BatchActive = false;
}

void InstancedRenderer::UpdateInstanceSSBO(const std::string& meshId)
{
    assert(!meshId.empty() && "Mesh ID cannot be empty");
    assert(m_MeshInstances.find(meshId) != m_MeshInstances.end() && "Mesh ID must exist in mesh instances");

    auto& meshInstances = m_MeshInstances[meshId];

    if (meshInstances.instances.empty()) {
        return;
    }

    // Upload full InstanceData (not just matrices)
    const auto& instances = meshInstances.instances;
    uint32_t instanceDataSize = static_cast<uint32_t>(instances.size() * sizeof(InstanceData));
    assert(instanceDataSize > 0 && "Instance data size must be positive");

    // Create or get existing SSBO for full instance data
    auto& ssbo = m_InstanceSSBOs[meshId];

    if (!ssbo) {
        ssbo = std::make_unique<ShaderStorageBuffer>(instances.data(), instanceDataSize, GL_DYNAMIC_DRAW);
        assert(ssbo && "Failed to create ShaderStorageBuffer");
        assert(ssbo->GetSSBOHandle() != 0 && "SSBO handle must be valid after creation");
    } else {
        // Update existing SSBO with full instance data
        assert(ssbo->GetSSBOHandle() != 0 && "Existing SSBO handle must be valid");
        ssbo->SetData(instances.data(), instanceDataSize, 0);
    }

    meshInstances.dirty = false;
}

void InstancedRenderer::RenderToPass(RenderPass& renderPass, const std::vector<RenderableData>& renderables, const FrameData& frameData, bool isOpaque)
{
    if (renderables.empty()) {
        // Clear cache if we transition from having objects to no objects
        if (!m_MeshInstances.empty())
        {
            Clear();
        }
        return;
    }

    // Only rebuild instance data when renderables actually change
    // This avoids redundant work for static scenes while handling dynamic changes
    bool changed = HasRenderablesChanged(renderables);

    if (changed)
    {
        BuildDynamicInstanceData(renderables);
    }

    // For transparent objects, sort batches by distance (back-to-front)
    // For opaque objects, render in any order (no sorting needed)
    if (!isOpaque) {
        // Build list of (meshId, furthest distance) pairs for transparent batches
        std::vector<std::pair<std::string, float>> batchDistances;
        batchDistances.reserve(m_MeshInstances.size());

        for (const auto& [meshId, batch] : m_MeshInstances) {
            if (batch.instances.empty()) {
                continue;
            }

            // Calculate the furthest instance distance in this batch
            // Using furthest distance ensures entire batch renders behind closer batches
            float maxDistance = 0.0f;
            for (const auto& instance : batch.instances) {
                // Extract position from model matrix (column 3: [3][0], [3][1], [3][2])
                glm::vec3 instancePos(
                    instance.modelMatrix[3][0],
                    instance.modelMatrix[3][1],
                    instance.modelMatrix[3][2]
                );
                float distance = glm::length(frameData.cameraPosition - instancePos);
                maxDistance = std::max(maxDistance, distance);
            }

            batchDistances.push_back({meshId, maxDistance});
        }

        // Sort batches by distance (furthest to nearest = back-to-front)
        std::sort(batchDistances.begin(), batchDistances.end(),
            [](const auto& a, const auto& b) {
                return a.second > b.second;  // Descending order (furthest first)
            });

        // Render batches in sorted order
        for (const auto& [meshId, distance] : batchDistances) {
            RenderInstancedMeshToPass(renderPass, meshId, frameData, isOpaque);
        }
    }
    else {
        // Opaque objects: render in any order (no sorting needed)
        for (const auto& pair : m_MeshInstances) {
            if (!pair.second.instances.empty()) {
                RenderInstancedMeshToPass(renderPass, pair.first, frameData, isOpaque);
            }
        }
    }
}

void InstancedRenderer::RenderShadowToPass(RenderPass& renderPass, const std::vector<RenderableData>& renderables, std::shared_ptr<Shader> shadowShader)
{
    if (renderables.empty() || !shadowShader) {
        return;
    }

    // Only rebuild instance data when renderables actually change
    // Shadow passes reuse the same instance data as the main pass
    if (HasRenderablesChanged(renderables))
    {
        BuildDynamicInstanceData(renderables);
    }

    // Render all instance batches with shadow shader (depth-only)
    for (const auto& pair : m_MeshInstances) {
        const auto& meshId = pair.first;
        const auto& meshInstances = pair.second;

        if (meshInstances.instances.empty()) {
            continue;
        }

        // Get SSBO for this mesh
        auto ssboIt = m_InstanceSSBOs.find(meshId);
        if (ssboIt == m_InstanceSSBOs.end()) {
            spdlog::error("No SSBO for mesh '{}' in shadow pass", meshId);
            continue;
        }

        // Get mesh
        if (!meshInstances.mesh) {
            spdlog::error("Missing mesh for '{}' in shadow pass", meshId);
            continue;
        }

        // 1. Bind shadow shader
        RenderCommands::BindShaderData bindShaderCmd{shadowShader};
        renderPass.Submit(bindShaderCmd);

        // 2. Bind instance SSBO
        RenderCommands::BindSSBOData bindSSBOCmd{
            ssboIt->second->GetSSBOHandle(),
            INSTANCE_SSBO_BINDING
        };
        renderPass.Submit(bindSSBOCmd);

        // Note: View and projection matrices are set by the shadow pass itself
        // We only need to bind SSBO and draw

        // 3. Draw all instances
        uint32_t indexCount = meshInstances.mesh->GetIndexCount();
        uint32_t instanceCount = static_cast<uint32_t>(meshInstances.instances.size());
        uint32_t vaoHandle = meshInstances.mesh->GetVertexArray()->GetVAOHandle();

        assert(indexCount > 0 && "Index count must be positive for instanced shadow drawing");
        assert(instanceCount > 0 && "Instance count must be positive for instanced shadow drawing");
        assert(vaoHandle != 0 && "VAO handle must be valid for instanced shadow drawing");

        RenderCommands::DrawElementsInstancedData drawCmd{
            vaoHandle,
            indexCount,
            instanceCount,
            0,  // Base instance
            GL_TRIANGLES
        };
        renderPass.Submit(drawCmd);
    }
}

void InstancedRenderer::BuildDynamicInstanceData(const std::vector<RenderableData>& renderables)
{
    // Clear and rebuild instance data based on currently visible renderables
    Clear();
    BeginInstanceBatch();

    // Group renderables by mesh for instancing
    for (size_t i = 0; i < renderables.size(); ++i) {
        const auto& renderable = renderables[i];
        if (!renderable.visible || !renderable.mesh || !renderable.material) {
            continue;
        }

        // Generate mesh ID from mesh+material pointers for proper instancing
        // Entities with identical mesh AND material will be batched together
        // This ensures different materials (especially blend modes) are in separate batches
        std::string meshId = std::to_string(reinterpret_cast<uintptr_t>(renderable.mesh.get()))
                           + "_" + std::to_string(reinterpret_cast<uintptr_t>(renderable.material.get()));

        // Add instance data with actual material properties
        InstanceData instanceData;
        instanceData.modelMatrix = renderable.transform;

        // Apply base material properties first
        glm::vec3 albedoColor = renderable.material->GetAlbedoColor();
        float metallicValue = renderable.material->GetMetallicValue();
        float roughnessValue = renderable.material->GetRoughnessValue();

        // Apply property block overrides if present
        if (renderable.propertyBlock) {
            glm::vec3 overrideColor;
            if (renderable.propertyBlock->TryGetVec3("u_AlbedoColor", overrideColor)) {
                albedoColor = overrideColor;

                // Debug: Log property block application (only first few times)
                static int propBlockApplyCount = 0;
                if (propBlockApplyCount < 3) {
                    spdlog::info("InstancedRenderer: Applied property block albedo color override ({}, {}, {})",
                        overrideColor.x, overrideColor.y, overrideColor.z);
                    propBlockApplyCount++;
                }
            }

            float overrideMetallic;
            if (renderable.propertyBlock->TryGetFloat("u_MetallicValue", overrideMetallic)) {
                metallicValue = overrideMetallic;
            }

            float overrideRoughness;
            if (renderable.propertyBlock->TryGetFloat("u_RoughnessValue", overrideRoughness)) {
                roughnessValue = overrideRoughness;
            }
        }

        instanceData.color = glm::vec4(albedoColor, 1.0f);
        instanceData.materialId = 0; // Not used yet, could be used for texture indexing
        instanceData.flags = 0;
        instanceData.metallic = metallicValue;
        instanceData.roughness = roughnessValue;

        // Set mesh data if not already set (use first material encountered for the mesh)
        if (m_MeshInstances.find(meshId) == m_MeshInstances.end()) {
            SetMeshData(meshId, renderable.mesh, renderable.material);
        }
        AddInstance(meshId, instanceData);
    }

    // End instance batch to update SSBOs
    EndInstanceBatch();
}

void InstancedRenderer::ForceRebuildCache()
{
    // Clear all tracking state to force rebuild on next RenderToPass()
    // This is called when components are updated in the editor (mesh/material changes)
    m_LastRenderableCount = 0;
    m_LastObjectIDs.clear();
    m_LastTransformHashes.clear();
    m_LastPropertyBlockHashes.clear();
    m_LastMaterialPointers.clear();
    m_LastMeshPointers.clear();
}

void InstancedRenderer::RenderInstancedMeshToPass(RenderPass& renderPass, const std::string& meshId, const FrameData& frameData, bool isOpaque)
{

    auto meshIt = m_MeshInstances.find(meshId);
    if (meshIt == m_MeshInstances.end()) {
        return;
    }

    if (meshIt->second.instances.empty()) {
        return;
    }

    const auto& meshInstances = meshIt->second;

    auto ssboIt = m_InstanceSSBOs.find(meshId);
    if (ssboIt == m_InstanceSSBOs.end()) {
        spdlog::error("No SSBO for mesh '{}'", meshId);
        return;
    }

    // Get mesh and material (these should be set from Scene data)
    if (!meshInstances.mesh) {
        spdlog::error("Missing mesh for '{}'", meshId);
        return;
    }

    if (!meshInstances.material) {
        spdlog::error("Missing material for '{}'", meshId);
        return;
    }

    // CRITICAL FIX: Filter by blend mode to prevent rendering objects in wrong pass
    // When building with ALL renderables, we need to skip batches that don't match the current pass
    bool materialIsOpaque = (meshInstances.material->GetBlendMode() == BlendingMode::Opaque);
    if (isOpaque != materialIsOpaque) {
        // Skip this batch - it belongs to the other pass
        return;
    }

    auto shader = meshInstances.material->GetShader();
    if (!shader) {
        spdlog::error("No shader for material in '{}'", meshId);
        return;
    }

    // Submit commands to the pass's isolated command buffer

    // 1. Bind shader
    RenderCommands::BindShaderData bindShaderCmd{shader};
    renderPass.Submit(bindShaderCmd);

    // 2. Bind instance SSBO
    RenderCommands::BindSSBOData bindSSBOCmd{
        ssboIt->second->GetSSBOHandle(),
        INSTANCE_SSBO_BINDING
    };
    renderPass.Submit(bindSSBOCmd);

    // 3. Set camera uniforms
    RenderCommands::SetUniformsData uniformsCmd{
        shader,
        glm::mat4(1.0f),  // Identity model matrix (instances have their own transforms)
        frameData.viewMatrix,
        frameData.projectionMatrix,
        frameData.cameraPosition
    };
    renderPass.Submit(uniformsCmd);

    RenderCommands::SetUniformBoolData opaqueCmd{
        shader, // shader
        "u_IsOpaquePass",                       // uniform name
        isOpaque
    };
	renderPass.Submit(opaqueCmd);

    // 4. Apply lighting setup via command submission (Option A - REFACTORED)
    if (m_PBRLighting) {
        // NEW: Submit lighting commands instead of direct OpenGL calls
        m_PBRLighting->SubmitLightingCommands(renderPass, shader, nullptr);

        // Submit unified shadow commands (includes all shadow types via SSBO)
        m_PBRLighting->SubmitShadowCommands(renderPass, shader, frameData);
    } else {
        spdlog::warn("PBRLightingRenderer not available for lighting setup");
    }


    // 6. Bind textures (if any)
	std::vector<Texture> materialTextures = meshInstances.material->GetAllTextures();
    if (materialTextures.size() > 0)
    {
        meshInstances.mesh->textures = materialTextures;
    }
    RenderCommands::BindTexturesData texturesCmd{meshInstances.mesh->textures, shader};
    renderPass.Submit(texturesCmd);

    // 7. Draw all instances
    uint32_t indexCount = meshInstances.mesh->GetIndexCount();
    uint32_t instanceCount = static_cast<uint32_t>(meshInstances.instances.size());
    uint32_t vaoHandle = meshInstances.mesh->GetVertexArray()->GetVAOHandle();

    assert(indexCount > 0 && "Index count must be positive for instanced drawing");
    assert(instanceCount > 0 && "Instance count must be positive for instanced drawing");
    assert(vaoHandle != 0 && "VAO handle must be valid for instanced drawing");

    RenderCommands::DrawElementsInstancedData drawCmd{
        vaoHandle,
        indexCount,
        instanceCount,
        0,  // Base instance
        GL_TRIANGLES  // Instanced rendering uses standard triangle meshes
    };
    renderPass.Submit(drawCmd);

}

void InstancedRenderer::SetMeshData(const std::string& meshId, const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material)
{
    assert(!meshId.empty() && "Mesh ID cannot be empty");
    assert(mesh && "Mesh cannot be null");
    assert(material && "Material cannot be null");
    assert(mesh->GetVertexArray() && "Mesh must have a valid vertex array");
    assert(mesh->GetVertexArray()->GetVAOHandle() != 0 && "Mesh VAO handle must be valid");
    assert(mesh->GetIndexCount() > 0 && "Mesh must have indices");

    if (!mesh) {
        spdlog::error("InstancedRenderer::SetMeshData: NULL mesh provided for '{}'", meshId);
        return;
    }

    if (!material) {
        spdlog::error("InstancedRenderer::SetMeshData: NULL material provided for '{}'", meshId);
        return;
    }

    auto& meshInstances = m_MeshInstances[meshId];
    meshInstances.mesh = mesh;
    meshInstances.material = material;
}

bool InstancedRenderer::HasRenderablesChanged(const std::vector<RenderableData> &renderables)
{
    // Quick check: different count means definitely changed
    if (renderables.size() != m_LastRenderableCount)
    {
        m_LastRenderableCount = renderables.size();
        UpdateAllTrackingData(renderables);
        return true;
    }

    // Same count - check if the object IDs match (detect entity replacement)
    // This catches cases like: delete entity A, add entity B (same count, different entities)
    if (m_LastObjectIDs.size() != renderables.size())
    {
        UpdateAllTrackingData(renderables);
        return true;
    }

    for (size_t i = 0; i < renderables.size(); ++i)
    {
        if (renderables[i].objectID != m_LastObjectIDs[i])
        {
            UpdateAllTrackingData(renderables);
            return true;
        }
    }

    // Check cheap things first (pointer comparisons) before expensive things (hashing)

    // Check if mesh pointers changed (mesh assignment/change in Inspector)
    if (m_LastMeshPointers.size() != renderables.size())
    {
        UpdateAllTrackingData(renderables);
        return true;
    }

    for (size_t i = 0; i < renderables.size(); ++i)
    {
        uintptr_t currentMeshPtr = reinterpret_cast<uintptr_t>(renderables[i].mesh.get());
        if (currentMeshPtr != m_LastMeshPointers[i])
        {
            UpdateAllTrackingData(renderables);
            return true;
        }
    }

    // Check if material pointers changed (material assignment in Inspector)
    if (m_LastMaterialPointers.size() != renderables.size())
    {
        UpdateAllTrackingData(renderables);
        return true;
    }

    for (size_t i = 0; i < renderables.size(); ++i)
    {
        uintptr_t currentMatPtr = reinterpret_cast<uintptr_t>(renderables[i].material.get());
        if (currentMatPtr != m_LastMaterialPointers[i])
        {
            UpdateAllTrackingData(renderables);
            return true;
        }
    }

    // Check if transforms changed (important for editor mode where entities move/scale/rotate)
    if (m_LastTransformHashes.size() != renderables.size())
    {
        UpdateAllTrackingData(renderables);
        return true;
    }

    for (size_t i = 0; i < renderables.size(); ++i)
    {
        // Hash transform matrix (all 16 elements for rotation-sensitive detection)
        const glm::mat4& t = renderables[i].transform;
        float currentHash = t[0][0] + t[0][1] + t[0][2] + t[0][3] +
                           t[1][0] + t[1][1] + t[1][2] + t[1][3] +
                           t[2][0] + t[2][1] + t[2][2] + t[2][3] +
                           t[3][0] + t[3][1] + t[3][2] + t[3][3];
        if (std::abs(currentHash - m_LastTransformHashes[i]) > 0.0001f)
        {
            UpdateAllTrackingData(renderables);
            return true;
        }
    }

    // Check property blocks last (most expensive due to map lookups)
    if (m_LastPropertyBlockHashes.size() != renderables.size())
    {
        UpdateAllTrackingData(renderables);
        return true;
    }

    for (size_t i = 0; i < renderables.size(); ++i)
    {
        const auto& renderable = renderables[i];

        // Compute hash of property block data
        float currentPropHash = 0.0f;
        if (renderable.propertyBlock)
        {
            // Hash albedo color if it exists
            glm::vec3 albedoColor(0.0f);
            if (renderable.propertyBlock->TryGetVec3("u_AlbedoColor", albedoColor))
            {
                currentPropHash += albedoColor.r + albedoColor.g + albedoColor.b;
            }

            // Hash metallic/roughness if they exist
            float metallic = 0.0f, roughness = 0.0f;
            if (renderable.propertyBlock->TryGetFloat("u_MetallicValue", metallic))
            {
                currentPropHash += metallic * 10.0f;
            }
            if (renderable.propertyBlock->TryGetFloat("u_RoughnessValue", roughness))
            {
                currentPropHash += roughness * 10.0f;
            }
        }

        // Compare with cached hash
        if (std::abs(currentPropHash - m_LastPropertyBlockHashes[i]) > 0.0001f)
        {
            UpdateAllTrackingData(renderables);
            return true;
        }
    }

    // No changes detected - same count, same IDs, same transforms, same properties, same materials, same meshes
    return false;
}

void InstancedRenderer::UpdateTransformHashes(const std::vector<RenderableData>& renderables)
{
    m_LastTransformHashes.clear();
    m_LastTransformHashes.reserve(renderables.size());
    for (const auto& r : renderables)
    {
        // Hash transform matrix (all 16 elements for rotation-sensitive detection)
        const glm::mat4& t = r.transform;
        float hash = t[0][0] + t[0][1] + t[0][2] + t[0][3] +
                    t[1][0] + t[1][1] + t[1][2] + t[1][3] +
                    t[2][0] + t[2][1] + t[2][2] + t[2][3] +
                    t[3][0] + t[3][1] + t[3][2] + t[3][3];
        m_LastTransformHashes.push_back(hash);
    }
}

void InstancedRenderer::UpdatePropertyBlockHashes(const std::vector<RenderableData>& renderables)
{
    m_LastPropertyBlockHashes.clear();
    m_LastPropertyBlockHashes.reserve(renderables.size());
    for (const auto& r : renderables)
    {
        // Hash property block data (if present)
        float hash = 0.0f;
        if (r.propertyBlock)
        {
            // Hash albedo color
            glm::vec3 albedoColor(0.0f);
            if (r.propertyBlock->TryGetVec3("u_AlbedoColor", albedoColor))
            {
                hash += albedoColor.r + albedoColor.g + albedoColor.b;
            }

            // Hash metallic/roughness
            float metallic = 0.0f, roughness = 0.0f;
            if (r.propertyBlock->TryGetFloat("u_MetallicValue", metallic))
            {
                hash += metallic * 10.0f;
            }
            if (r.propertyBlock->TryGetFloat("u_RoughnessValue", roughness))
            {
                hash += roughness * 10.0f;
            }
        }
        m_LastPropertyBlockHashes.push_back(hash);
    }
}

void InstancedRenderer::UpdateMaterialPointers(const std::vector<RenderableData>& renderables)
{
    m_LastMaterialPointers.clear();
    m_LastMaterialPointers.reserve(renderables.size());
    for (const auto& r : renderables)
    {
        // Store material pointer as uintptr_t for change detection
        uintptr_t matPtr = reinterpret_cast<uintptr_t>(r.material.get());
        m_LastMaterialPointers.push_back(matPtr);
    }
}

void InstancedRenderer::UpdateMeshPointers(const std::vector<RenderableData>& renderables)
{
    m_LastMeshPointers.clear();
    m_LastMeshPointers.reserve(renderables.size());
    for (const auto& r : renderables)
    {
        // Store mesh pointer as uintptr_t for change detection
        uintptr_t meshPtr = reinterpret_cast<uintptr_t>(r.mesh.get());
        m_LastMeshPointers.push_back(meshPtr);
    }
}

void InstancedRenderer::UpdateAllTrackingData(const std::vector<RenderableData>& renderables)
{
    // Update object IDs
    m_LastObjectIDs.clear();
    m_LastObjectIDs.reserve(renderables.size());
    for (const auto& r : renderables)
    {
        m_LastObjectIDs.push_back(r.objectID);
    }

    // Update mesh pointers
    m_LastMeshPointers.clear();
    m_LastMeshPointers.reserve(renderables.size());
    for (const auto& r : renderables)
    {
        uintptr_t meshPtr = reinterpret_cast<uintptr_t>(r.mesh.get());
        m_LastMeshPointers.push_back(meshPtr);
    }

    // Update material pointers
    m_LastMaterialPointers.clear();
    m_LastMaterialPointers.reserve(renderables.size());
    for (const auto& r : renderables)
    {
        uintptr_t matPtr = reinterpret_cast<uintptr_t>(r.material.get());
        m_LastMaterialPointers.push_back(matPtr);
    }

    // Update transform hashes
    m_LastTransformHashes.clear();
    m_LastTransformHashes.reserve(renderables.size());
    for (const auto& r : renderables)
    {
        // Hash transform matrix (all 16 elements for rotation-sensitive detection)
        const glm::mat4& t = r.transform;
        float hash = t[0][0] + t[0][1] + t[0][2] + t[0][3] +
                    t[1][0] + t[1][1] + t[1][2] + t[1][3] +
                    t[2][0] + t[2][1] + t[2][2] + t[2][3] +
                    t[3][0] + t[3][1] + t[3][2] + t[3][3];
        m_LastTransformHashes.push_back(hash);
    }

    // Update property block hashes
    m_LastPropertyBlockHashes.clear();
    m_LastPropertyBlockHashes.reserve(renderables.size());
    for (const auto& r : renderables)
    {
        float hash = 0.0f;
        if (r.propertyBlock)
        {
            glm::vec3 albedoColor(0.0f);
            if (r.propertyBlock->TryGetVec3("u_AlbedoColor", albedoColor))
            {
                hash += albedoColor.r + albedoColor.g + albedoColor.b;
            }

            float metallic = 0.0f, roughness = 0.0f;
            if (r.propertyBlock->TryGetFloat("u_MetallicValue", metallic))
            {
                hash += metallic * 10.0f;
            }
            if (r.propertyBlock->TryGetFloat("u_RoughnessValue", roughness))
            {
                hash += roughness * 10.0f;
            }
        }
        m_LastPropertyBlockHashes.push_back(hash);
    }
}

