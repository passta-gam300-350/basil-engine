#pragma once

#include "../Buffer/ShaderStorageBuffer.h"
#include "../Utility/Camera.h"
#include "../Core/Renderer.h"
#include "../Core/RenderCommandBuffer.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>

// Forward declarations to avoid cross-library dependencies
class Scene;
class Mesh;
class Material;

// GPU Instanced Rendering using SSBO - integrates with existing architecture
class InstancedRenderer {
public:
    // Instance data that will be stored in SSBO (std430 aligned)
    struct InstanceData {
        glm::mat4 modelMatrix;      // 64 bytes
        glm::vec4 color;            // 16 bytes
        uint32_t materialId;        // 4 bytes
        uint32_t flags;             // 4 bytes
        float metallic;             // 4 bytes
        float roughness;            // 4 bytes
        // Total: 96 bytes per instance (properly aligned)
    };
    
    // Light data structures
    struct PointLight {
        glm::vec3 position;
        glm::vec3 color;
        float intensity;
        float constant;
        float linear;
        float quadratic;
    };
    
    struct DirectionalLight {
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
    };
    
    struct SpotLight {
        glm::vec3 position;
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
        float cutOff;
        float outerCutOff;
        float constant;
        float linear;
        float quadratic;
    };
    
public:
    InstancedRenderer();
    ~InstancedRenderer();
    
    // Instance management
    void BeginInstanceBatch();
    void AddInstance(const std::string& meshId, const InstanceData& instance);
    void EndInstanceBatch();
    void Clear();
    
    // Rendering using existing command buffer system
    void Render(Scene* scene, Camera& camera);
    
    // Mesh and material setup
    void SetMeshData(const std::string& meshId, std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);
    
    // Light management
    void ClearLights();
    void AddPointLight(const PointLight& light);
    void AddDirectionalLight(const DirectionalLight& light);
    void AddSpotLight(const SpotLight& light);
    
    // Configuration
    void SetMaxInstances(uint32_t maxInstances) { m_MaxInstances = maxInstances; }
    uint32_t GetInstanceCount() const { return m_TotalInstances; }
    
    // SSBO binding points (for shader access)
    static constexpr uint32_t INSTANCE_SSBO_BINDING = 0;
    
private:
    // Instance data storage per mesh
    struct MeshInstances {
        std::vector<InstanceData> instances;
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Material> material;
        bool dirty = true;
    };
    
    std::unordered_map<std::string, MeshInstances> m_MeshInstances;
    std::unordered_map<std::string, std::unique_ptr<ShaderStorageBuffer>> m_InstanceSSBOs;
    
    // Light storage
    std::vector<PointLight> m_PointLights;
    std::vector<DirectionalLight> m_DirectionalLights;
    std::vector<SpotLight> m_SpotLights;
    
    uint32_t m_MaxInstances;
    uint32_t m_TotalInstances;
    bool m_BatchActive;
    
    void UpdateInstanceSSBO(const std::string& meshId);
    void RenderInstancedMesh(const std::string& meshId, Camera& camera);
};