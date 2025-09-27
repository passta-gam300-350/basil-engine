#pragma once

#include "RenderPass.h"
#include "../Utility/RenderData.h"
#include <memory>

// Forward declarations
class Shader;

/**
 * Picking Render Pass - Handles object picking via color-coded rendering
 *
 * Renders objects with unique colors representing their object IDs to a
 * special framebuffer. This allows mouse picking by reading pixel colors
 * and converting them back to object IDs.
 */
class PickingRenderPass : public RenderPass
{
public:
    PickingRenderPass();
    PickingRenderPass(const PickingRenderPass&) = delete;
    PickingRenderPass& operator=(const PickingRenderPass&) = delete;
    PickingRenderPass(PickingRenderPass&&) = delete;
    PickingRenderPass& operator=(PickingRenderPass&&) = delete;
    ~PickingRenderPass() override = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

    // Set picking shader (should output object ID as color)
    void SetPickingShader(const std::shared_ptr<Shader>& shader) { m_PickingShader = shader; }

    // Picking query interface
    PickingResult QueryPicking(const MousePickingQuery& query, const RenderContext& context);

    // Enable/disable picking pass
    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

private:
    // Render objects with their unique ID colors
    void RenderPickingData(RenderContext& context);

    // Convert screen coordinates to framebuffer coordinates
    void ScreenToFramebuffer(int screenX, int screenY, int screenWidth, int screenHeight,
                            int& fbX, int& fbY) const;

    // Convert object ID to color and vice versa
    glm::vec3 ObjectIDToColor(uint32_t objectID) const;
    uint32_t ColorToObjectID(const glm::vec3& color) const;

    // Shader storage
    std::shared_ptr<Shader> m_PickingShader;

    // Pass state
    bool m_Enabled = false;  // Disabled by default (only run when needed)

    static constexpr uint8_t PICKING_PASS_ID = 10;  // Execute after main passes when needed
};