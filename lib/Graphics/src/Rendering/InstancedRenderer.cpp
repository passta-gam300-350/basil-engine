#include "Rendering/InstancedRenderer.h"
#include "Rendering/PBRLightingRenderer.h"
#include "Utility/RenderData.h"
#include "Resources/Mesh.h"
#include "Resources/Material.h"
#include <iostream>

InstancedRenderer::InstancedRenderer()
    : m_MaxInstances(10000), m_TotalInstances(0), m_BatchActive(false)
{
}

InstancedRenderer::~InstancedRenderer()
{
    Clear();
}

void InstancedRenderer::BeginInstanceBatch()
{
    if (m_BatchActive) {
        //std::cerr << "Warning: Instance batch already active" << std::endl;
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
    if (!m_BatchActive) {
        std::cerr << "Error: Must call BeginInstanceBatch() first" << std::endl;
        return;
    }
    
    if (m_TotalInstances >= m_MaxInstances) {
        std::cerr << "Warning: Exceeded maximum instances (" << m_MaxInstances << ")" << std::endl;
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
        std::cerr << "Warning: No active instance batch" << std::endl;
        return;
    }
    
    m_BatchActive = false;
    
    // Update SSBOs for dirty meshes
    for (auto& pair : m_MeshInstances) {
        if (pair.second.dirty && !pair.second.instances.empty()) {
            UpdateInstanceSSBO(pair.first);
        }
    }
    
    /*std::cout << "InstancedRenderer: Batch completed with " << m_TotalInstances << " instances across " 
              << m_MeshInstances.size() << " mesh types" << std::endl;*/
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
    auto& meshInstances = m_MeshInstances[meshId];
    
    if (meshInstances.instances.empty()) {
        return;
    }
    
    // Extract only the transform matrices
    std::vector<glm::mat4> matrices;
    matrices.reserve(meshInstances.instances.size());
    
    for (const auto& instance : meshInstances.instances) {
        matrices.push_back(instance.modelMatrix);
    }
    
    // Create or get existing SSBO for matrices only
    auto& ssbo = m_InstanceSSBOs[meshId];
    uint32_t matrixDataSize = matrices.size() * sizeof(glm::mat4);
    
    if (!ssbo) {
        ssbo = std::make_unique<ShaderStorageBuffer>(matrices.data(), matrixDataSize, GL_DYNAMIC_DRAW);
    } else {
        // Update existing SSBO with matrices only
        ssbo->SetData(matrices.data(), matrixDataSize, 0);
    }
    
    meshInstances.dirty = false;
    
    /*std::cout << "InstancedRenderer: Updated SSBO for '" << meshId << "' with " 
              << matrices.size() << " transform matrices" << std::endl;*/
}

void InstancedRenderer::Render(const std::vector<RenderableData>& renderables, Camera& camera)
{
    if (renderables.empty()) {
        return;
    }
    
    // Clear existing instances and rebuild from submitted renderables
    Clear();
    
    // Group renderables by mesh for instancing
    for (const auto& renderable : renderables) {
        if (!renderable.visible || !renderable.mesh || !renderable.material) {
            continue;
        }
        
        // Generate a mesh ID from the mesh pointer for grouping
        std::string meshId = std::to_string(reinterpret_cast<uintptr_t>(renderable.mesh.get()));
        
        // Add instance data
        InstanceData instanceData;
        instanceData.modelMatrix = renderable.transform;
        instanceData.color = glm::vec4(1.0f); // Default color
        instanceData.materialId = 0; // Default material ID
        instanceData.flags = 0;
        instanceData.metallic = 0.5f; // Default PBR values
        instanceData.roughness = 0.5f;
        
        // Set mesh data if not already set
        SetMeshData(meshId, renderable.mesh, renderable.material);
        AddInstance(meshId, instanceData);
    }
    
    // Render each instanced mesh type
    for (const auto& pair : m_MeshInstances) {
        if (!pair.second.instances.empty()) {
            RenderInstancedMesh(pair.first, camera);
        }
    }
}

void InstancedRenderer::RenderInstancedMesh(const std::string& meshId, Camera& camera)
{
    //std::cout << "InstancedRenderer::RenderInstancedMesh called for '" << meshId << "'" << std::endl;
    
    auto meshIt = m_MeshInstances.find(meshId);
    if (meshIt == m_MeshInstances.end()) {
        //std::cerr << "Error: No mesh instances found for '" << meshId << "'" << std::endl;
        //std::cout << "Available mesh IDs: ";
        for (const auto& pair : m_MeshInstances) {
            std::cout << "'" << pair.first << "' ";
        }
        std::cout << std::endl;
        return;
    }
    
    if (meshIt->second.instances.empty()) {
        //std::cerr << "Error: No instances for mesh '" << meshId << "'" << std::endl;
        return;
    }
    
    const auto& meshInstances = meshIt->second;
    //std::cout << "Found " << meshInstances.instances.size() << " instances for '" << meshId << "'" << std::endl;
    
    auto ssboIt = m_InstanceSSBOs.find(meshId);
    if (ssboIt == m_InstanceSSBOs.end()) {
        std::cerr << "Error: No SSBO for mesh '" << meshId << "'" << std::endl;
        return;
    }
    
    // Get mesh and material (these should be set from Scene data)
    if (!meshInstances.mesh) {
        std::cerr << "Error: Missing mesh for '" << meshId << "'" << std::endl;
        return;
    }
    
    if (!meshInstances.material) {
        std::cerr << "Error: Missing material for '" << meshId << "'" << std::endl;
        return;
    }
    
    auto shader = meshInstances.material->GetShader();
    if (!shader) {
        std::cerr << "Error: No shader for material in '" << meshId << "'" << std::endl;
        return;
    }
    
    // Generate sort key for command batching
    RenderCommands::CommandSortKey sortKey;
    sortKey.pass = 0;  // Main opaque pass
    sortKey.material = reinterpret_cast<uintptr_t>(meshInstances.material.get()) & 0xFFFFFF;
    sortKey.mesh = reinterpret_cast<uintptr_t>(meshInstances.mesh.get()) & 0xFFFF;
    sortKey.instance = 0;  // All instances rendered together
    
    // Submit commands using existing architecture
    
    // 1. Bind shader
    RenderCommands::BindShaderData bindShaderCmd{shader};
    Renderer::Get().Submit(bindShaderCmd, sortKey);
    
    // 2. Bind instance SSBO
    RenderCommands::BindSSBOData bindSSBOCmd{
        ssboIt->second->GetSSBOHandle(),
        INSTANCE_SSBO_BINDING
    };
    Renderer::Get().Submit(bindSSBOCmd, sortKey);
    
    // 3. Set camera uniforms
    RenderCommands::SetUniformsData uniformsCmd{
        shader,
        glm::mat4(1.0f),  // Identity model matrix (instances have their own transforms)
        camera.GetViewMatrix(),
        camera.GetProjectionMatrix(),
        camera.GetPosition()
    };
    Renderer::Get().Submit(uniformsCmd, sortKey);
    
    // 4. Apply PBR lighting using the independent lighting system
    auto* pbrLighting = PBRLightingRenderer::GetInstance();
    if (pbrLighting) {
        // Create material properties from mesh data
        PBRLightingRenderer::PBRMaterialProperties material;
        material.albedoColor = glm::vec3(0.8f, 0.7f, 0.6f);  // Warm metallic base
        material.metallicValue = 0.7f;                       // More metallic for better reflections
        material.roughnessValue = 0.3f;                      // Smoother for better light interaction
        material.hasTexture = !meshInstances.mesh->textures.empty();
        
        // Apply lighting to shader
        pbrLighting->ApplyLightingToShader(shader, material);
    } else {
        std::cerr << "Warning: PBRLightingRenderer not available for lighting setup" << std::endl;
    }
    
    // 5. Bind textures (if any)
    RenderCommands::BindTexturesData texturesCmd{meshInstances.mesh->textures, shader};
    Renderer::Get().Submit(texturesCmd, sortKey);
    
    // 5. Draw all instances
    RenderCommands::DrawElementsInstancedData drawCmd{
        meshInstances.mesh->GetVertexArray()->GetVAOHandle(),
        meshInstances.mesh->GetIndexCount(),
        static_cast<uint32_t>(meshInstances.instances.size()),
        0  // Base instance
    };
    Renderer::Get().Submit(drawCmd, sortKey);
    
    /*std::cout << "InstancedRenderer: Submitted instanced draw for '" << meshId 
              << "' with " << meshInstances.instances.size() << " instances" << std::endl;*/
}

void InstancedRenderer::SetMeshData(const std::string& meshId, std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
{
    if (!mesh) {
        std::cerr << "InstancedRenderer::SetMeshData: NULL mesh provided for '" << meshId << "'" << std::endl;
        return;
    }
    
    if (!material) {
        std::cerr << "InstancedRenderer::SetMeshData: NULL material provided for '" << meshId << "'" << std::endl;
        return;
    }
    
    auto& meshInstances = m_MeshInstances[meshId];
    meshInstances.mesh = mesh;
    meshInstances.material = material;
    
    std::cout << "InstancedRenderer: Successfully set mesh and material data for '" << meshId << "'" << std::endl;
    std::cout << "  - Mesh vertices: " << mesh->GetVertexCount() << std::endl;
    std::cout << "  - Mesh indices: " << mesh->GetIndexCount() << std::endl;
    std::cout << "  - Material name: " << material->GetName() << std::endl;
}


