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
class MaterialPropertyBlock;
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
        float normalStrength;       // 4 bytes
        float padding;              // 4 bytes
        float padding2;             // 4 bytes (pad to 112 bytes - multiple of 16)
        float padding3;             // 4 bytes
        // Total: 112 bytes per instance (properly aligned to 16-byte boundary)
    };

    struct ResolvedMaterialData {
        glm::vec3 albedoColor = glm::vec3(0.8f, 0.7f, 0.6f);
        float metallic = 0.7f;
        float roughness = 0.3f;
        float normalStrength = 1.0f;
    };
    
    
public:
    InstancedRenderer(PBRLightingRenderer* lighting);
    ~InstancedRenderer();
    
    // Instance management
    void BeginInstanceBatch();
    void AddInstance(uint64_t meshId, const InstanceData& instance);
    void EndInstanceBatch();
    void Clear();

    // Helper to compute mesh ID from mesh + material pointers (zero allocations)
    static inline uint64_t ComputeMeshId(const void *meshPtr, const void *materialPtr)
    {
        uint64_t h = reinterpret_cast<uintptr_t>(meshPtr);
        uint64_t mat = reinterpret_cast<uintptr_t>(materialPtr);
        h ^= mat + 0x9E3779B97F4A7C15ULL + (h << 12) + (h >> 4);
        return h;
    }

    static ResolvedMaterialData ResolvePerInstanceMaterialData(
        const Material* material,
        const MaterialPropertyBlock* propertyBlock);

    static bool AreResolvedMaterialDataEqual(
        const ResolvedMaterialData& lhs,
        const ResolvedMaterialData& rhs,
        float epsilon = 0.0001f);

    static bool AreTransformsEqual(
        const glm::mat4& lhs,
        const glm::mat4& rhs,
        float epsilon = 0.0001f);

    // Rendering using pass-isolated command buffers
    void RenderToPass(RenderPass& renderPass, const std::vector<RenderableData>& renderables, const FrameData& frameData, bool isOpaque);

    // Shadow-specific rendering (depth-only, no lighting)
    void RenderShadowToPass(RenderPass& renderPass, const std::vector<RenderableData>& renderables, std::shared_ptr<Shader> shadowShader);

    // Build dynamic instance data based on visible renderables
    void BuildDynamicInstanceData(const std::vector<RenderableData>& renderables);

    // Force rebuild of cached instance data (called when components are updated in editor)
    void ForceRebuildCache();

    // Reset change tracking for new frame (call at frame start to enable per-frame caching)
    void ResetFrameChangeTracking();

    // Mesh and material setup
    void SetMeshData(uint64_t meshId, const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material);

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
        std::vector<Texture> cachedTextures;  // OPTIMIZED: Cache material textures to avoid repeated GetAllTextures() allocations
        bool dirty = true;
        bool texturesCached = false;  // Track if cachedTextures is up-to-date
    };
    
    std::unordered_map<uint64_t, MeshInstances> m_MeshInstances;
    std::unordered_map<uint64_t, std::unique_ptr<ShaderStorageBuffer>> m_InstanceSSBOs;

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
    std::vector<glm::mat4> m_LastTransforms;  // Cache full transforms so scale/rotation edits invalidate instance data
    std::vector<ResolvedMaterialData> m_LastResolvedMaterialData;  // Track effective per-instance material data
    std::vector<uintptr_t> m_LastMaterialPointers;  // Cache material pointers for material change detection
    std::vector<uintptr_t> m_LastMeshPointers;  // Cache mesh pointers for mesh change detection
    bool m_ChangeCheckDoneThisFrame = false;  // OPTIMIZED: Only check changes once per frame
    bool m_LastFrameHadChanges = false;  // OPTIMIZED: Cache result of last check

    void UpdateInstanceSSBO(uint64_t meshId);
    void RenderInstancedMeshToPass(RenderPass& renderPass, uint64_t meshId, const FrameData& frameData, bool isOpaque);
    void SubmitFogCommands(RenderPass& renderPass, std::shared_ptr<Shader> shader, const FrameData& frameData);
    bool HasRenderablesChanged(const std::vector<RenderableData> &renderables);
    void UpdateAllTrackingData(const std::vector<RenderableData>& renderables);
};
