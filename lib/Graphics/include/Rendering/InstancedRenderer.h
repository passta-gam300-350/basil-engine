/******************************************************************************/
/*!
\file   InstancedRenderer.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the InstancedRenderer class for GPU instanced rendering using SSBO

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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
    void RenderToPass(RenderPass& renderPass, const std::vector<RenderableData>& renderables, const FrameData& frameData, bool isOpaque);

    // Shadow-specific rendering (depth-only, no lighting)
    void RenderShadowToPass(RenderPass& renderPass, const std::vector<RenderableData>& renderables, std::shared_ptr<Shader> shadowShader);

    // Build dynamic instance data based on visible renderables
    void BuildDynamicInstanceData(const std::vector<RenderableData>& renderables);

    // Force rebuild of cached instance data (called when components are updated in editor)
    void ForceRebuildCache();

    // Mesh and material setup
    void SetMeshData(const std::string& meshId, const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material);
    
    // Render skinned meshes (called after regular instanced rendering)
    void RenderSkinnedMeshes(RenderPass& renderPass, const FrameData& frameData);


    // Configuration
    void SetMaxInstances(uint32_t maxInstances) { m_MaxInstances = maxInstances; }
    uint32_t GetInstanceCount() const { return m_TotalInstances; }
    // Upload bone matrices for skinned mesh rendering
    void UploadBoneMatrices(const glm::mat4* matrices, uint32_t count);
    // SSBO binding points (for shader access)
    static constexpr uint32_t INSTANCE_SSBO_BINDING = 0;
    // Bone matrix SSBO for skeletal animation
    std::unique_ptr<ShaderStorageBuffer> m_BoneMatrixSSBO;
    // Instance SSBO for skinned meshes (single-element, reused per skinned draw)
    std::unique_ptr<ShaderStorageBuffer> m_SkinnedInstanceSSBO;
    static constexpr uint32_t BONE_SSBO_BINDING = 2;
    static constexpr uint32_t MAX_TOTAL_BONES = 4096;
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

    // Skinned renderables (rendered individually, not batched)
    std::vector<const RenderableData*> m_SkinnedRenderables;

    uint32_t m_MaxInstances;
    uint32_t m_TotalInstances;
    bool m_BatchActive;

    // Injected dependencies
    PBRLightingRenderer* m_PBRLighting;

    // Change detection for efficient rebuilding
    size_t m_LastRenderableCount = 0;
    std::vector<uint32_t> m_LastObjectIDs;
    std::vector<float> m_LastTransformHashes;  // Cache transform hashes for change detection
    std::vector<float> m_LastPropertyBlockHashes;  // Cache property block hashes for MaterialOverrides changes
    std::vector<uintptr_t> m_LastMaterialPointers;  // Cache material pointers for material change detection
    std::vector<uintptr_t> m_LastMeshPointers;  // Cache mesh pointers for mesh change detection

    void UpdateInstanceSSBO(const std::string& meshId);
    void RenderInstancedMeshToPass(RenderPass& renderPass, const std::string& meshId, const FrameData& frameData, bool isOpaque);
    bool HasRenderablesChanged(const std::vector<RenderableData> &renderables);
    void UpdateTransformHashes(const std::vector<RenderableData>& renderables);
    void UpdatePropertyBlockHashes(const std::vector<RenderableData>& renderables);
    void UpdateMaterialPointers(const std::vector<RenderableData>& renderables);
    void UpdateMeshPointers(const std::vector<RenderableData>& renderables);
    void UpdateAllTrackingData(const std::vector<RenderableData>& renderables);
};