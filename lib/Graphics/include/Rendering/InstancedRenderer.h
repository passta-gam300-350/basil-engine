#pragma once

#include "../Buffer/ShaderStorageBuffer.h"
#include "PBRLightingRenderer.h"
#include "../Core/RenderCommandBuffer.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>

// Forward declarations to avoid cross-library dependencies
struct RenderableData;
struct FrameData;
class Mesh;
class Material;
class RenderPass;

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
    
    
public:
    InstancedRenderer(PBRLightingRenderer* lighting);
    ~InstancedRenderer();
    
    // Instance management
    void BeginInstanceBatch();
    void AddInstance(const std::string& meshId, const InstanceData& instance);
    void EndInstanceBatch();
    void Clear();

    // Rendering using pass-isolated command buffers
    void RenderToPass(RenderPass& renderPass, const std::vector<RenderableData>& renderables, const FrameData& frameData);

    // Shadow-specific rendering (depth-only, no lighting)
    void RenderShadowToPass(RenderPass& renderPass, const std::vector<RenderableData>& renderables, std::shared_ptr<Shader> shadowShader);

    // Build dynamic instance data based on visible renderables
    void BuildDynamicInstanceData(const std::vector<RenderableData>& renderables);
    
    // Mesh and material setup
    void SetMeshData(const std::string& meshId, std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);
    
    
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



    uint32_t m_MaxInstances;
    uint32_t m_TotalInstances;
    bool m_BatchActive;

    // Injected dependencies
    PBRLightingRenderer* m_PBRLighting;

    void UpdateInstanceSSBO(const std::string& meshId);
    void RenderInstancedMeshToPass(RenderPass& renderPass, const std::string& meshId, const FrameData& frameData);
};