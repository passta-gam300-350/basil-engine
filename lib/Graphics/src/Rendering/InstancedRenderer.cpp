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
    uint32_t instanceDataSize = instances.size() * sizeof(InstanceData);
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

void InstancedRenderer::RenderToPass(RenderPass& renderPass, const std::vector<RenderableData>& renderables, const FrameData& frameData)
{
    if (renderables.empty()) {
        return;
    }

    // Always rebuild instance data based on currently visible renderables
    // This handles dynamic frustum culling as camera moves
    BuildDynamicInstanceData(renderables);

    // Render the instance batches to the specified pass
    for (const auto& pair : m_MeshInstances) {
        if (!pair.second.instances.empty()) {
            RenderInstancedMeshToPass(renderPass, pair.first, frameData);
        }
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

        // Generate mesh ID from mesh pointer for proper instancing
        // Identical mesh pointers (shared meshes) will automatically be batched together
        std::string meshId = std::to_string(reinterpret_cast<uintptr_t>(renderable.mesh.get()));

        // Add instance data with actual material properties
        InstanceData instanceData;
        instanceData.modelMatrix = renderable.transform;
        instanceData.color = glm::vec4(renderable.material->GetAlbedoColor(), 1.0f);
        instanceData.materialId = 0; // Not used yet, could be used for texture indexing
        instanceData.flags = 0;
        instanceData.metallic = renderable.material->GetMetallicValue();
        instanceData.roughness = renderable.material->GetRoughnessValue();

        // Set mesh data if not already set (use first material encountered for the mesh)
        if (m_MeshInstances.find(meshId) == m_MeshInstances.end()) {
            SetMeshData(meshId, renderable.mesh, renderable.material);
        }
        AddInstance(meshId, instanceData);
    }

    // End instance batch to update SSBOs
    EndInstanceBatch();
}

void InstancedRenderer::RenderInstancedMeshToPass(RenderPass& renderPass, const std::string& meshId, const FrameData& frameData)
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

    // 4. Apply lighting setup (material properties are now per-instance in SSBO)
    if (m_PBRLighting) {
        // Apply lighting uniforms only (no per-instance material data)
        m_PBRLighting->ApplyLightingToShader(shader, nullptr);  // Pass null since materials are per-instance
    } else {
        spdlog::warn("PBRLightingRenderer not available for lighting setup");
    }

    // 5. Set directional shadow mapping uniforms if available
    if (!frameData.shadowMaps.empty() && !frameData.shadowMatrices.empty() && frameData.shadowMaps[0]) {
        uint32_t shadowTexID = frameData.shadowMaps[0]->GetDepthAttachmentRendererID();

        RenderCommands::SetShadowUniformsData shadowCmd{
            shader,
            frameData.shadowMatrices[0],
            shadowTexID,
            8,  // Use texture unit 8 for shadow map (TEXTURE_SLOT_SHADOW)
            true  // Enable shadows
        };
        renderPass.Submit(shadowCmd);
    } else {
        // No shadow data available - disable shadow mapping
        // Bind a default/dummy shadow matrix and unbind shadow texture
        glm::mat4 identityMatrix = glm::mat4(1.0f);

        RenderCommands::SetShadowUniformsData disableShadowCmd{
            shader,
            identityMatrix,
            0,  // Bind texture ID 0 (unbind)
            8,  // Use texture unit 8 for shadow map (TEXTURE_SLOT_SHADOW)
            false  // Disable shadows
        };
        renderPass.Submit(disableShadowCmd);
    }

    // 5b. Set point shadow cubemaps if available
    shader->use();  // Ensure shader is active

    // Always initialize ALL 4 samplerCube uniforms to prevent undefined behavior
    // Even unused samplers must be bound to valid texture units
    const int MAX_POINT_SHADOW_MAPS = 4;
    int activeShadowMaps = (!frameData.pointShadowCubemaps.empty() && !frameData.pointShadowFarPlanes.empty())
                           ? static_cast<int>(frameData.pointShadowCubemaps.size())
                           : 0;

    shader->setInt("u_NumPointShadowMaps", activeShadowMaps);

    for (int i = 0; i < MAX_POINT_SHADOW_MAPS; ++i) {
        uint32_t textureUnit = 9 + static_cast<uint32_t>(i);
        glActiveTexture(GL_TEXTURE0 + textureUnit);

        if (i < activeShadowMaps) {
            // Bind actual shadow cubemap
            glBindTexture(GL_TEXTURE_CUBE_MAP, frameData.pointShadowCubemaps[i]);

            // Set far plane uniform
            std::string farPlaneName = "u_PointShadowFarPlanes[" + std::to_string(i) + "]";
            shader->setFloat(farPlaneName, frameData.pointShadowFarPlanes[i]);
        } else {
            // Bind dummy cubemap (texture ID 0) for unused slots
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        }

        // Always set sampler uniform to proper texture unit
        std::string samplerName = "u_PointShadowMaps[" + std::to_string(i) + "]";
        shader->setInt(samplerName, static_cast<int>(textureUnit));
    }


    // 6. Bind textures (if any)
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

void InstancedRenderer::SetMeshData(const std::string& meshId, std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
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


