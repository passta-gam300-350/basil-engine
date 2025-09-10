#include "Rendering/InstancedRenderer.h"
#include "../../../test/examples/lib/Graphics/Engine/Scene/Scene.h"
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

void InstancedRenderer::Render(Scene* scene, Camera& camera)
{
    if (!scene || m_MeshInstances.empty()) {
        return;
    }
    
    //std::cout << "InstancedRenderer: Rendering " << m_MeshInstances.size() << " instanced meshes" << std::endl;
    
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
    
    // 4. Set material uniforms
    shader->use(); // Ensure shader is active for uniform setting
    shader->setVec3("u_LightPos", glm::vec3(10.0f, 10.0f, 10.0f));
    shader->setVec3("u_LightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    shader->setVec3("u_BaseColor", glm::vec3(1.0f, 1.0f, 1.0f));  // White to show textures
    shader->setFloat("u_Metallic", 0.1f);
    shader->setFloat("u_Roughness", 0.6f);
    
    // Enable texture if mesh has diffuse texture
    if (!meshInstances.mesh->textures.empty()) {
        shader->setBool("u_HasTexture0", true);
        shader->setInt("u_Texture0", 0);  // Texture unit 0
    } else {
        shader->setBool("u_HasTexture0", false);
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