#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "../Core/Renderer.h"

// Forward declarations
struct RenderableData;
struct FrameData;

// Rendering coordinator - generates render commands from submitted data
// Owned by SceneRenderer - this is graphics-specific
class MeshRenderer
{
public:
    MeshRenderer(Renderer* renderer);
    ~MeshRenderer() = default;

    // Rendering coordination - takes renderable data and generates render commands
    // This is graphics logic, no dependency on any specific data storage
    void Render(const std::vector<RenderableData>& renderables, const FrameData& frameData);

private:
    void GenerateDrawCommand(const RenderableData& renderable,
                           const glm::mat4& viewMatrix,
                           const glm::mat4& projectionMatrix,
                           const glm::vec3& cameraPosition);

    Renderer* m_Renderer;
};