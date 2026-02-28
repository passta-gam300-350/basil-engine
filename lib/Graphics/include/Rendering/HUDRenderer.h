/******************************************************************************/
/*!
\file   HUDRenderer.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/06
\brief    Declares the HUDRenderer class for rendering screen-space UI/HUD elements

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "../Buffer/ShaderStorageBuffer.h"
#include "../Utility/HUDData.h"
#include "../Resources/Shader.h"
#include <memory>
#include <vector>
#include <map>
#include <utility>
#include <glm/glm.hpp>

// Forward declarations
class RenderPass;
struct FrameData;

/**
 * @brief HUD/UI Renderer using GPU instancing
 *
 * Renders screen-space UI elements with orthographic projection.
 * Batches elements by texture for efficient GPU instancing.
 * Supports layering, rotation, and alpha blending.
 */
class HUDRenderer {
public:
    /**
     * @brief Instance data for SSBO (std430 layout)
     *
     * Each HUD element becomes one instance in the GPU.
     * Batched by texture for efficient rendering.
     */
    struct HUDInstanceData {
        glm::vec2 position;         // Screen position (pixels)
        glm::vec2 size;             // Size (pixels)
        glm::vec4 color;            // Tint color
        float rotation;             // Rotation (radians)
        uint32_t anchor;            // Anchor point (cast from HUDAnchor)
        float layer;                // Normalized layer depth (0.0=back, 1.0=front)
        float _pad;                 // Alignment padding
        // Total: 48 bytes (aligned to vec4)
    };

public:
    HUDRenderer();
    ~HUDRenderer();

    // Element submission API
    void BeginFrame();
    void SubmitElement(const HUDElementData& element);
    void EndFrame();

    // Rendering
    void RenderToPass(RenderPass& renderPass, const FrameData& frameData);

    // Shader configuration
    void SetHUDShader(const std::shared_ptr<Shader>& shader) { m_HUDShader = shader; }

    // Reference resolution configuration
    void SetReferenceResolution(const glm::vec2& resolution) { m_ReferenceResolution = resolution; }
    glm::vec2 GetReferenceResolution() const { return m_ReferenceResolution; }

    // Stats
    uint32_t GetElementCount() const { return m_TotalElements; }
    uint32_t GetBatchCount() const { return static_cast<uint32_t>(m_TextureBatches.size()); }

    // SSBO binding point
    static constexpr uint32_t HUD_INSTANCE_SSBO_BINDING = 3;

private:
    // Batch storage per texture
    struct TextureBatch {
        uint32_t textureID;
        std::vector<HUDInstanceData> instances;
        std::unique_ptr<ShaderStorageBuffer> ssbo;
        bool dirty = true;
    };

    // Texture batches keyed by (layer, textureID) — map order gives correct back-to-front draw sequence
    std::map<std::pair<uint8_t, uint32_t>, TextureBatch> m_TextureBatches;

    // HUD shader (orthographic projection)
    std::shared_ptr<Shader> m_HUDShader;

    // Quad mesh (shared by all HUD elements)
    uint32_t m_QuadVAO = 0;
    uint32_t m_QuadVBO = 0;

    // Fixed reference resolution for HUD layout (default: 1920x1080)
    // HUD element positions are interpreted in this coordinate space
    glm::vec2 m_ReferenceResolution = glm::vec2(1920.0f, 1080.0f);

    // Stats
    uint32_t m_TotalElements = 0;

    // Internal methods
    void CreateQuadMesh();
    void UpdateBatchSSBO(TextureBatch& batch);
    void RenderBatch(RenderPass& renderPass, const TextureBatch& batch, const FrameData& frameData);
};
