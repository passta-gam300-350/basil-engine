/******************************************************************************/
/*!
\file   InstancedRenderer.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Instanced renderer implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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

    // OPTIMIZED: Clear existing instances but KEEP vector capacity and mesh/material data
    // This reuses allocated memory instead of deallocating and reallocating
    for (auto& pair : m_MeshInstances) {
        pair.second.instances.clear();  // Keeps capacity, only resets size
        pair.second.dirty = true;
    }
}

void InstancedRenderer::AddInstance(uint64_t meshId, const InstanceData& instance)
{
    assert(m_BatchActive && "Must call BeginInstanceBatch() first");
    assert(meshId != 0 && "Mesh ID cannot be zero");
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

    // OPTIMIZED: Pre-reserve if empty (avoid growth reallocations)
    if (meshInstances.instances.empty() && meshInstances.instances.capacity() == 0) {
        meshInstances.instances.reserve(64);  // Reasonable default for new batches
    }

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
    m_SkinnedRenderables.clear();

    // Also reset tracking state so HasRenderablesChanged() returns true
    // after a clear, forcing BuildDynamicInstanceData() to repopulate.
    m_LastRenderableCount = 0;
    m_LastObjectIDs.clear();
    m_LastTransformHashes.clear();
    m_LastPropertyBlockPointers.clear();
    m_LastMaterialPointers.clear();
    m_LastMeshPointers.clear();
    m_ChangeCheckDoneThisFrame = false;
    m_LastFrameHadChanges = false;
}

void InstancedRenderer::UpdateInstanceSSBO(uint64_t meshId)
{
    assert(meshId != 0 && "Mesh ID cannot be zero");
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

        if (instanceDataSize > ssbo->GetSize())
        {
            // Resize if new data exceeds current buffer size (reallocates GPU memory)
            ssbo->Resize(instanceDataSize);
		}
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
        std::vector<std::pair<uint64_t, float>> batchDistances;
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
    // NOTE: RenderSkinnedMeshes uses direct GL calls and must be called
    // AFTER ExecuteCommands() in the render pass, not here (deferred).
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
            spdlog::error("No SSBO for mesh ID in shadow pass");
            continue;
        }

        // Get mesh
        if (!meshInstances.mesh) {
            spdlog::error("Missing mesh for mesh ID in shadow pass");
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

        // 3. Set face culling based on material cull mode (shadows respect material culling)
        if (meshInstances.material) {
            CullMode cullMode = meshInstances.material->GetCullMode();
            RenderCommands::SetFaceCullingData cullCmd;
            switch (cullMode) {
                case CullMode::Back:
                    cullCmd.enable = true;
                    cullCmd.cullFace = GL_BACK;
                    break;
                case CullMode::Front:
                    cullCmd.enable = true;
                    cullCmd.cullFace = GL_FRONT;
                    break;
                case CullMode::Off:
                    cullCmd.enable = false;
                    break;
            }
            renderPass.Submit(cullCmd);
        }

        // 4. Draw all instances
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
    // OPTIMIZED: Incremental rebuild instead of full clear
    // Only clear instance lists, keep SSBOs and mesh data for reuse
    BeginInstanceBatch();

    // Clear skinned renderables (rebuilt below from current frame's data)
    m_SkinnedRenderables.clear();
    m_SkinnedRenderables.reserve(renderables.size() / 10);  // Pre-reserve (estimate 10% skinned)

    // Group renderables by mesh for instancing
    for (size_t i = 0; i < renderables.size(); ++i) {
        const auto& renderable = renderables[i];
        if (!renderable.visible || !renderable.mesh || !renderable.material) {
            continue;
        }

        if (renderable.isSkinned)
        {
            m_SkinnedRenderables.push_back(&renderable);
            continue;
        }

        // OPTIMIZED: Use numeric hash instead of string concatenation (ZERO allocations!)
        // Entities with identical mesh AND material will be batched together
        // This ensures different materials (especially blend modes) are in separate batches
        uint64_t meshId = ComputeMeshId(renderable.mesh.get(), renderable.material.get());

        // Add instance data with actual material properties
        InstanceData instanceData;
        instanceData.modelMatrix = renderable.transform;

        // Apply base material properties first
        glm::vec3 albedoColor = renderable.material->GetAlbedoColor();
        float metallicValue = renderable.material->GetMetallicValue();
        float roughnessValue = renderable.material->GetRoughnessValue();
        float normalStrength = renderable.material->GetNormalStrength();

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

            float overrideNormalStrength;
            if (renderable.propertyBlock->TryGetFloat("u_NormalStrength", overrideNormalStrength)) {
                normalStrength = overrideNormalStrength;
            }
        }

        instanceData.color = glm::vec4(albedoColor, 1.0f);
        instanceData.materialId = 0; // Not used yet, could be used for texture indexing
        instanceData.flags = 0;
        instanceData.metallic = metallicValue;
        instanceData.roughness = roughnessValue;
        instanceData.normalStrength = normalStrength;
        instanceData.padding = 0.0f;
        instanceData.padding2 = 0.0f;
        instanceData.padding3 = 0.0f;

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
    m_LastPropertyBlockPointers.clear();
    m_LastMaterialPointers.clear();
    m_LastMeshPointers.clear();
    m_ChangeCheckDoneThisFrame = false;
    m_LastFrameHadChanges = false;
}

void InstancedRenderer::ResetFrameChangeTracking()
{
    // OPTIMIZATION: Call this at the start of each frame to reset the "already checked" flag
    // This allows HasRenderablesChanged() to be called multiple times per frame
    // (for different passes) but only do the expensive check once
    m_ChangeCheckDoneThisFrame = false;
}

void InstancedRenderer::RenderInstancedMeshToPass(RenderPass& renderPass, uint64_t meshId, const FrameData& frameData, bool isOpaque)
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
        spdlog::error("No SSBO for mesh ID");
        return;
    }

    // Get mesh and material (these should be set from Scene data)
    if (!meshInstances.mesh) {
        spdlog::error("Missing mesh for mesh ID");
        return;
    }

    if (!meshInstances.material) {
        spdlog::error("Missing material for mesh ID");
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

    // Disable skinning for non-skinned batched meshes
    RenderCommands::SetUniformBoolData disableSkinningCmd{ shader, "u_EnableSkinning", false };
    renderPass.Submit(disableSkinningCmd);

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

    // 5. Submit fog commands (OGLDev Tutorial 39-style)
    SubmitFogCommands(renderPass, shader, frameData);

    // 6. Set face culling based on material cull mode
    CullMode cullMode = meshInstances.material->GetCullMode();
    RenderCommands::SetFaceCullingData cullCmd;
    switch (cullMode) {
        case CullMode::Back:
            cullCmd.enable = true;
            cullCmd.cullFace = GL_BACK;
            break;
        case CullMode::Front:
            cullCmd.enable = true;
            cullCmd.cullFace = GL_FRONT;
            break;
        case CullMode::Off:
            cullCmd.enable = false;
            break;
    }
    renderPass.Submit(cullCmd);

    // 7. Bind textures (OPTIMIZED: use cached textures to avoid GetAllTextures() allocation)
    if (!meshInstances.cachedTextures.empty())
    {
        meshInstances.mesh->textures = meshInstances.cachedTextures;
    }
    RenderCommands::BindTexturesData texturesCmd{meshInstances.mesh->textures, shader};
    renderPass.Submit(texturesCmd);

    // 8. Draw all instances
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

void InstancedRenderer::SetMeshData(uint64_t meshId, const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material)
{
    assert(meshId != 0 && "Mesh ID cannot be zero");
    assert(mesh && "Mesh cannot be null");
    assert(material && "Material cannot be null");
    assert(mesh->GetVertexArray() && "Mesh must have a valid vertex array");
    assert(mesh->GetVertexArray()->GetVAOHandle() != 0 && "Mesh VAO handle must be valid");
    assert(mesh->GetIndexCount() > 0 && "Mesh must have indices");

    if (!mesh) {
        spdlog::error("InstancedRenderer::SetMeshData: NULL mesh provided for mesh ID");
        return;
    }

    if (!material) {
        spdlog::error("InstancedRenderer::SetMeshData: NULL material provided for mesh ID");
        return;
    }

    auto& meshInstances = m_MeshInstances[meshId];
    meshInstances.mesh = mesh;
    meshInstances.material = material;

    // OPTIMIZED: Pre-cache material textures to avoid repeated GetAllTextures() calls
    meshInstances.cachedTextures = material->GetAllTextures();
    meshInstances.texturesCached = true;
}

void InstancedRenderer::RenderSkinnedMeshes(RenderPass& renderPass, const FrameData& frameData)
{
    if (m_SkinnedRenderables.empty())
    {
        return;
    }

    // Reset lighting/shadow caches so skinned meshes get full lighting setup.
    // After the main batch's ExecuteCommands() + CleanupGPUState(), the shader
    // program was unbound. Uniforms persist in the program object, but we
    // force re-submission to be safe.
    if (m_PBRLighting)
    {
        m_PBRLighting->ResetFrameCache();
    }

    static int logCount = 0;
    bool shouldLog = false;

    for (const RenderableData* renderable : m_SkinnedRenderables)
    {
        if (!renderable || !renderable->mesh || !renderable->material)
            continue;
        if (!renderable->visible)
            continue;
        if (!renderable->boneMatrices || renderable->boneCount == 0)
            continue;

        auto shader = renderable->material->GetShader();
        if (!shader)
            continue;

        // Each skinned mesh gets its own submit-then-execute cycle
        // so that immediate GL calls (SSBO uploads) and deferred commands stay in sync.

        // 1. Upload bone matrices (immediate GL: creates/updates SSBO, binds to point 2)
        UploadBoneMatrices(renderable->boneMatrices, renderable->boneCount);

        // 2. Upload instance data (immediate GL: creates/updates SSBO, binds to point 0)
        {
            InstanceData instanceData{};
            instanceData.modelMatrix = renderable->transform;

            glm::vec3 albedo = renderable->material->GetAlbedoColor();
            instanceData.color = glm::vec4(albedo, 1.0f);
            instanceData.metallic = renderable->material->GetMetallicValue();
            instanceData.roughness = renderable->material->GetRoughnessValue();
            instanceData.normalStrength = renderable->material->GetNormalStrength();
            instanceData.materialId = 0;
            instanceData.flags = 0;
            instanceData.padding = 0.0f;
            instanceData.padding2 = 0.0f;
            instanceData.padding3 = 0.0f;

            if (!m_SkinnedInstanceSSBO)
            {
                m_SkinnedInstanceSSBO = std::make_unique<ShaderStorageBuffer>(
                    &instanceData, uint32_t(sizeof(InstanceData)), GL_DYNAMIC_DRAW);
            }
            else
            {
                m_SkinnedInstanceSSBO->SetData(&instanceData, sizeof(InstanceData), 0);
            }
            m_SkinnedInstanceSSBO->BindBase(INSTANCE_SSBO_BINDING);
        }

        // 3. Submit deferred commands: shader, uniforms, lighting, textures, draw

        // Set blend state based on material blend mode
        bool isTransparent = renderable->material->GetBlendMode() == BlendingMode::Transparent;
        if (isTransparent)
        {
            renderPass.Submit(RenderCommands::SetDepthTestData{ true, GL_LESS, false });
            renderPass.Submit(RenderCommands::SetBlendingData{ true, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA });
        }
        else
        {
            renderPass.Submit(RenderCommands::SetDepthTestData{ true, GL_LESS, true });
            renderPass.Submit(RenderCommands::SetBlendingData{ false });
        }

        // Set face culling based on material cull mode
        CullMode cullMode = renderable->material->GetCullMode();
        RenderCommands::SetFaceCullingData cullCmd;
        switch (cullMode) {
            case CullMode::Back:
                cullCmd.enable = true;
                cullCmd.cullFace = GL_BACK;
                break;
            case CullMode::Front:
                cullCmd.enable = true;
                cullCmd.cullFace = GL_FRONT;
                break;
            case CullMode::Off:
                cullCmd.enable = false;
                break;
        }
        renderPass.Submit(cullCmd);

        RenderCommands::BindShaderData bindShaderCmd{ shader };
        renderPass.Submit(bindShaderCmd);

        RenderCommands::SetUniformBoolData enableSkinningCmd{ shader, "u_EnableSkinning", true };
        renderPass.Submit(enableSkinningCmd);

        renderPass.Submit(RenderCommands::SetUniformBoolData{ shader, "u_SpritesheetMode", renderable->isSpritesheetMode });

        RenderCommands::SetUniformIntData boneOffsetCmd{ shader, "u_BoneOffset", 0 };
        renderPass.Submit(boneOffsetCmd);

        renderPass.Submit(RenderCommands::SetUniformBoolData{ shader, "u_IsOpaquePass", !isTransparent });

        RenderCommands::SetUniformsData uniformsCmd{
            shader,
            renderable->transform,
            frameData.viewMatrix,
            frameData.projectionMatrix,
            frameData.cameraPosition
        };
        renderPass.Submit(uniformsCmd);

        if (m_PBRLighting)
        {
            m_PBRLighting->SubmitLightingCommands(renderPass, shader, nullptr);
            m_PBRLighting->SubmitShadowCommands(renderPass, shader, frameData);
        }

        std::vector<Texture> textures = renderable->material->GetAllTextures();
        RenderCommands::BindTexturesData texturesCmd{ textures, shader };
        renderPass.Submit(texturesCmd);

        // Re-bind SSBOs via deferred commands (belt-and-suspenders: the immediate
        // binds above may have been overwritten by CleanupGPUState or other passes)
        renderPass.Submit(RenderCommands::BindSSBOData{
            m_SkinnedInstanceSSBO->GetSSBOHandle(), INSTANCE_SSBO_BINDING });
        renderPass.Submit(RenderCommands::BindSSBOData{
            m_BoneMatrixSSBO->GetSSBOHandle(), BONE_SSBO_BINDING });

        uint32_t vaoHandle = renderable->mesh->GetVertexArray()->GetVAOHandle();
        uint32_t indexCount = renderable->mesh->GetIndexCount();

        if (shouldLog)
            spdlog::info("[Skinned] Drawing: VAO={}, indices={}, bones={}, ssbo0={}, ssbo2={}, transform[3]=({},{},{})",
                vaoHandle, indexCount, renderable->boneCount,
                m_SkinnedInstanceSSBO->GetSSBOHandle(), m_BoneMatrixSSBO->GetSSBOHandle(),
                renderable->transform[3][0], renderable->transform[3][1], renderable->transform[3][2]);

        RenderCommands::DrawElementsInstancedData drawCmd{
            vaoHandle,
            indexCount,
            1,
            0,
            GL_TRIANGLES
        };
        renderPass.Submit(drawCmd);

        // 4. Execute immediately - SSBOs are bound, commands reference them now
        renderPass.ExecuteCommands();
        renderPass.ClearCommands();

        // 5. Disable skinning for next draw (immediate)
        shader->use();
        shader->setBool("u_EnableSkinning", false);
        shader->setBool("u_SpritesheetMode", false);
    }

    // Skinned draws execute outside the main pass's normal state restoration path.
    // Restore the default scene state so transparent spritesheet smoke cannot leak
    // blend/depth/cull settings into later passes or subsequent frames.
    renderPass.Submit(RenderCommands::SetBlendingData{ false });
    renderPass.Submit(RenderCommands::SetDepthTestData{ true, GL_LESS, true });
    renderPass.Submit(RenderCommands::SetFaceCullingData{ true, GL_BACK });
    renderPass.ExecuteCommands();
    renderPass.ClearCommands();
}

bool InstancedRenderer::HasRenderablesChanged(const std::vector<RenderableData> &renderables)
{
    // OPTIMIZATION: If already checked this frame, return cached result
    // This avoids redundant checks when called multiple times per frame (shadow pass, main pass, etc.)
    if (m_ChangeCheckDoneThisFrame)
    {
        return m_LastFrameHadChanges;
    }

    // Quick check: different count means definitely changed
    if (renderables.size() != m_LastRenderableCount)
    {
        m_LastRenderableCount = renderables.size();
        UpdateAllTrackingData(renderables);
        m_ChangeCheckDoneThisFrame = true;
        m_LastFrameHadChanges = true;
        return true;
    }

    // OPTIMIZATION: Fast path for empty scenes
    if (renderables.empty())
    {
        m_ChangeCheckDoneThisFrame = true;
        m_LastFrameHadChanges = false;
        return false;
    }

    // Same count - check if the object IDs match (detect entity replacement)
    if (m_LastObjectIDs.size() != renderables.size())
    {
        UpdateAllTrackingData(renderables);
        m_ChangeCheckDoneThisFrame = true;
        m_LastFrameHadChanges = true;
        return true;
    }

    // OPTIMIZATION: Single-pass check of all cheap comparisons (better cache locality)
    for (size_t i = 0; i < renderables.size(); ++i)
    {
        const auto& r = renderables[i];

        // Check object ID (cheap)
        if (r.objectID != m_LastObjectIDs[i])
        {
            UpdateAllTrackingData(renderables);
            m_ChangeCheckDoneThisFrame = true;
            m_LastFrameHadChanges = true;
            return true;
        }

        // Check mesh pointer (cheap)
        uintptr_t meshPtr = reinterpret_cast<uintptr_t>(r.mesh.get());
        if (meshPtr != m_LastMeshPointers[i])
        {
            UpdateAllTrackingData(renderables);
            m_ChangeCheckDoneThisFrame = true;
            m_LastFrameHadChanges = true;
            return true;
        }

        // Check material pointer (cheap)
        uintptr_t matPtr = reinterpret_cast<uintptr_t>(r.material.get());
        if (matPtr != m_LastMaterialPointers[i])
        {
            UpdateAllTrackingData(renderables);
            m_ChangeCheckDoneThisFrame = true;
            m_LastFrameHadChanges = true;
            return true;
        }

        // OPTIMIZED: Only check property block POINTER, not contents (much faster!)
        // If user wants per-frame property changes to trigger rebuild, they can call ForceRebuildCache()
        uintptr_t propBlockPtr = reinterpret_cast<uintptr_t>(r.propertyBlock.get());
        if (propBlockPtr != m_LastPropertyBlockPointers[i])
        {
            UpdateAllTrackingData(renderables);
            m_ChangeCheckDoneThisFrame = true;
            m_LastFrameHadChanges = true;
            return true;
        }

        // OPTIMIZED: Simplified transform hash (only position, not full matrix)
        // This is 75% faster than summing all 16 elements
        const glm::mat4& t = r.transform;
        float currentHash = t[3][0] + t[3][1] + t[3][2];  // Just position (translation column)
        if (std::abs(currentHash - m_LastTransformHashes[i]) > 0.0001f)
        {
            UpdateAllTrackingData(renderables);
            m_ChangeCheckDoneThisFrame = true;
            m_LastFrameHadChanges = true;
            return true;
        }
    }

    // No changes detected
    m_ChangeCheckDoneThisFrame = true;
    m_LastFrameHadChanges = false;
    return false;
}

void InstancedRenderer::UpdateAllTrackingData(const std::vector<RenderableData>& renderables)
{
    // OPTIMIZED: Single-pass update with pre-reserved capacity (better cache locality)
    const size_t count = renderables.size();

    m_LastObjectIDs.clear();
    m_LastObjectIDs.reserve(count);

    m_LastMeshPointers.clear();
    m_LastMeshPointers.reserve(count);

    m_LastMaterialPointers.clear();
    m_LastMaterialPointers.reserve(count);

    m_LastTransformHashes.clear();
    m_LastTransformHashes.reserve(count);

    m_LastPropertyBlockPointers.clear();
    m_LastPropertyBlockPointers.reserve(count);

    // Single loop to update all tracking data
    for (const auto& r : renderables)
    {
        m_LastObjectIDs.push_back(r.objectID);
        m_LastMeshPointers.push_back(reinterpret_cast<uintptr_t>(r.mesh.get()));
        m_LastMaterialPointers.push_back(reinterpret_cast<uintptr_t>(r.material.get()));
        m_LastPropertyBlockPointers.push_back(reinterpret_cast<uintptr_t>(r.propertyBlock.get()));

        // OPTIMIZED: Simplified transform hash (position only, 75% faster)
        const glm::mat4& t = r.transform;
        m_LastTransformHashes.push_back(t[3][0] + t[3][1] + t[3][2]);
    }
}


void InstancedRenderer::UploadBoneMatrices(const glm::mat4* matrices, uint32_t count)
{
    if (matrices == nullptr || count == 0)
        return;

    // Create SSBO if it doesn't exist
    if (!m_BoneMatrixSSBO)
    {
        m_BoneMatrixSSBO = std::make_unique<ShaderStorageBuffer>(
            nullptr,
            MAX_TOTAL_BONES * uint32_t(sizeof(glm::mat4)),
            GL_DYNAMIC_DRAW
        );
    }

    // Upload bone matrices
    uint32_t dataSize = count * sizeof(glm::mat4);
    m_BoneMatrixSSBO->SetData(matrices, dataSize, 0);

    // Bind to shader binding point
    m_BoneMatrixSSBO->BindBase(BONE_SSBO_BINDING);
}

// ===== FOG RENDERING (OGLDev Tutorial 39-style) =====

void InstancedRenderer::SubmitFogCommands(RenderPass& renderPass, std::shared_ptr<Shader> shader, const FrameData& frameData)
{
    // Check if fog data is available
    if (!frameData.fogData) {
        return;  // No fog data, skip
    }

    const FogData& fog = *frameData.fogData;

    // Submit fog type uniform
    RenderCommands::SetUniformIntData fogTypeCmd{
        shader,
        "u_FogType",
        static_cast<int>(fog.type)
    };
    renderPass.Submit(fogTypeCmd);

    // Submit fog parameters (only if fog is enabled)
    if (fog.IsEnabled()) {
        // Submit fog start distance (Linear fog only, but always set for simplicity)
        RenderCommands::SetUniformFloatData fogStartCmd{
            shader,
            "u_FogStart",
            fog.start
        };
        renderPass.Submit(fogStartCmd);

        // Submit fog end distance (all fog types use this)
        RenderCommands::SetUniformFloatData fogEndCmd{
            shader,
            "u_FogEnd",
            fog.end
        };
        renderPass.Submit(fogEndCmd);

        // Submit fog density (Exponential fog types only, but always set)
        RenderCommands::SetUniformFloatData fogDensityCmd{
            shader,
            "u_FogDensity",
            fog.density
        };
        renderPass.Submit(fogDensityCmd);

        // Submit fog color
        RenderCommands::SetUniformVec3Data fogColorCmd{
            shader,
            "u_FogColor",
            fog.color
        };
        renderPass.Submit(fogColorCmd);
    }
}
