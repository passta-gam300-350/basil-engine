/******************************************************************************/
/*!
\file   RenderContext.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the RenderContext struct for passing rendering data between pipeline stages

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "../Utility/FrameData.h"
#include "../Utility/RenderData.h"
#include <vector>
#include <glm/glm.hpp>

// Forward declarations
class Renderer;
class InstancedRenderer;
class PBRLightingRenderer;
class ResourceManager;
class TextureSlotManager;
class ParticleRenderer;
/**
 * RenderContext - Single source of truth for all rendering data
 *
 * Eliminates data duplication by providing const references to scene data
 * and mutable access to shared frame data. This context is passed to all
 * pipelines and passes, ensuring consistent data access without copying.
 *
 * Design principles:
 * - Scene data is read-only (const references)
 * - FrameData is mutable for inter-pass communication
 * - Rendering systems are non-owning references
 * - No data copying between pipeline levels
 */
struct RenderContext {
    // Scene data (read-only) - owned by SceneRenderer
    const std::vector<RenderableData>& renderables;
    const std::vector<SubmittedLightData>& lights;
    const glm::vec3& ambientLight;

    // Shared frame data (mutable) - communication between passes
    FrameData& frameData;

    // Rendering systems (non-owning references)
    InstancedRenderer& instancedRenderer;
    PBRLightingRenderer& pbrLighting;
    ResourceManager& resourceManager;
    TextureSlotManager& textureSlotManager;
    ParticleRenderer& particleRenderer;

    // HDR pipeline data (mutable) - set by HDR passes
    uint32_t hdrTextureID = 0;        // Set by MainRenderingPass
    float avgLuminance = 0.5f;        // Set by HDRLuminancePass
    float exposure = 1.0f;            // Set by HDRLuminancePass

    // Constructor
    RenderContext(
        const std::vector<RenderableData>& renderables_,
        const std::vector<SubmittedLightData>& lights_,
        const glm::vec3& ambientLight_,
        FrameData& frameData_,
        InstancedRenderer& instancedRenderer_,
        PBRLightingRenderer& pbrLighting_,
        ResourceManager& resourceManager_,
        TextureSlotManager& textureSlotManager_,
        ParticleRenderer& particleRenderer_
    ) : renderables(renderables_)
      , lights(lights_)
      , ambientLight(ambientLight_)
      , frameData(frameData_)
      , instancedRenderer(instancedRenderer_)
      , pbrLighting(pbrLighting_)
      , resourceManager(resourceManager_)
      , textureSlotManager(textureSlotManager_) 
      , particleRenderer(particleRenderer_){}

    // Delete copy constructor and assignment to prevent accidental copying
    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;

    // Allow move operations
    RenderContext(RenderContext&&) = default;
    RenderContext& operator=(RenderContext&&) = default;
};