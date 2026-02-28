/******************************************************************************/
/*!
\file   WorldUIRenderer.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/02/15
\brief  Declares the WorldUIRenderer class for rendering world-space UI elements

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "../Buffer/ShaderStorageBuffer.h"
#include "../Utility/WorldUIData.h"
#include "../Resources/Shader.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

// Forward declarations
class RenderPass;
struct FrameData;

/**
 * @brief World-space UI renderer using GPU instancing
 *
 * Renders world-space UI quads (textured or solid color) with billboard support.
 * Batches elements by texture for efficient GPU instancing.
 * Supports Full, Cylindrical, and None billboard modes.
 */
class WorldUIRenderer {
public:
    /**
     * @brief Instance data for SSBO (std430 layout)
     *
     * Each world UI element becomes one instance in the GPU.
     * Billboard basis vectors are pre-calculated on CPU.
     */
    struct WorldUIInstanceData {
        glm::vec3 worldPosition;    float _pad0;    // 16 bytes
        glm::vec3 billboardRight;   float _pad1;    // 16 bytes
        glm::vec3 billboardUp;      float _pad2;    // 16 bytes
        glm::vec2 size;             glm::vec2 _pad3;// 16 bytes
        glm::vec4 uvRect;                           // 16 bytes (min.xy, max.zw)
        glm::vec4 color;                            // 16 bytes
        // Total: 96 bytes
    };

public:
    WorldUIRenderer();
    ~WorldUIRenderer();

    // Element submission API
    void BeginFrame();
    void SubmitElement(const WorldUIElementData& element);
    void EndFrame();

    // Rendering
    void RenderToPass(RenderPass& renderPass, const FrameData& frameData);

    // Shader configuration
    void SetWorldUIShader(const std::shared_ptr<Shader>& shader) { m_WorldUIShader = shader; }

    // Stats
    uint32_t GetElementCount() const { return m_TotalElements; }
    uint32_t GetBatchCount() const { return static_cast<uint32_t>(m_TextureBatches.size()); }

    // SSBO binding point
    static constexpr uint32_t WORLD_UI_INSTANCE_SSBO_BINDING = 5;

private:
    // Batch storage per texture
    struct TextureBatch {
        uint32_t textureID;
        std::vector<WorldUIInstanceData> instances;
        std::unique_ptr<ShaderStorageBuffer> ssbo;
        bool dirty = true;
    };

    // Texture batches (key = textureID, 0 = no texture)
    std::unordered_map<uint32_t, TextureBatch> m_TextureBatches;

    // World UI shader
    std::shared_ptr<Shader> m_WorldUIShader;

    // Quad mesh (shared by all world UI elements)
    uint32_t m_QuadVAO = 0;
    uint32_t m_QuadVBO = 0;

    // Stats
    uint32_t m_TotalElements = 0;

    // Internal methods
    void CreateQuadMesh();
    void UpdateBatchSSBO(TextureBatch& batch);
    void RenderBatch(RenderPass& renderPass, const TextureBatch& batch, const FrameData& frameData);

    // Billboard calculation
    void CalculateBillboardVectors(
        const WorldUIElementData& element,
        glm::vec3& outRight,
        glm::vec3& outUp
    ) const;
};
