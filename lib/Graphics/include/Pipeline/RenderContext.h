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

    // Constructor
    RenderContext(
        const std::vector<RenderableData>& renderables_,
        const std::vector<SubmittedLightData>& lights_,
        const glm::vec3& ambientLight_,
        FrameData& frameData_,
        InstancedRenderer& instancedRenderer_,
        PBRLightingRenderer& pbrLighting_,
        ResourceManager& resourceManager_
    ) : renderables(renderables_)
      , lights(lights_)
      , ambientLight(ambientLight_)
      , frameData(frameData_)
      , instancedRenderer(instancedRenderer_)
      , pbrLighting(pbrLighting_)
      , resourceManager(resourceManager_) {}

    // Delete copy constructor and assignment to prevent accidental copying
    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;

    // Allow move operations
    RenderContext(RenderContext&&) = default;
    RenderContext& operator=(RenderContext&&) = default;
};