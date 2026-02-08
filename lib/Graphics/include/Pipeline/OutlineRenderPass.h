/******************************************************************************/
/*!
\file   OutlineRenderPass.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Outline render pass for selection highlighting

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "RenderPass.h"
#include <memory>
#include <unordered_set>

// Forward declarations
class Shader;

/**
 * Outline Rendering Pass - Renders outlines using stencil testing
 *
 * This pass implements the two-pass stencil buffer technique for object outlines:
 * 1. First pass: Render selected objects normally, writing to stencil buffer
 * 2. Second pass: Render scaled versions where stencil buffer != reference value
 *
 * The pass renders to the main color buffer after the main rendering pass,
 * overlaying outlines on top of the scene.
 */
class OutlineRenderPass : public RenderPass
{
public:
    OutlineRenderPass();
    OutlineRenderPass(std::shared_ptr<Shader> outlineShader);
    OutlineRenderPass(const OutlineRenderPass&) = delete;
    OutlineRenderPass& operator=(const OutlineRenderPass&) = delete;
    OutlineRenderPass(OutlineRenderPass&&) = delete;
    OutlineRenderPass& operator=(OutlineRenderPass&&) = delete;
    ~OutlineRenderPass() override = default;

    // Context-based execution
    void Execute(RenderContext& context) override;

    // Shader configuration
    void SetOutlineShader(const std::shared_ptr<Shader>& shader) { m_OutlineShader = shader; }

    // Outline selection - which objects should be outlined
    void AddOutlinedObject(uint32_t objectID) { m_OutlinedObjects.insert(objectID); }
    void RemoveOutlinedObject(uint32_t objectID) { m_OutlinedObjects.erase(objectID); }
    void ClearOutlinedObjects() { m_OutlinedObjects.clear(); }
    bool IsObjectOutlined(uint32_t objectID) const { return m_OutlinedObjects.count(objectID) > 0; }
    const std::unordered_set<uint32_t>& GetOutlinedObjects() const { return m_OutlinedObjects; }

    // Outline appearance configuration
    void SetOutlineColor(const glm::vec3& color) { m_OutlineColor = color; }
    glm::vec3 GetOutlineColor() const { return m_OutlineColor; }

    void SetOutlineScale(float scale) { m_OutlineScale = scale; }
    float GetOutlineScale() const { return m_OutlineScale; }

    // Enable/disable outline rendering
    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

private:
    // Two-pass rendering implementation
    void RenderFirstPass(RenderContext& context);   // Draw objects to stencil buffer
    void RenderSecondPass(RenderContext& context);  // Draw scaled outlines

    // Shader for outline rendering
    std::shared_ptr<Shader> m_OutlineShader;

    // Set of object IDs that should be outlined
    std::unordered_set<uint32_t> m_OutlinedObjects;

    // Outline appearance settings
    glm::vec3 m_OutlineColor = glm::vec3(1.0f, 0.5f, 0.0f);  // Default: Orange
    float m_OutlineScale = 1.05f;                             // Default: 5% larger

    // Enable/disable toggle
    bool m_Enabled = true;
};
