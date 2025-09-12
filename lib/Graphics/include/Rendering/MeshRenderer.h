#pragma once

#include "../Utility/Camera.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

// Forward declarations
struct RenderableData;

// Rendering coordinator - generates render commands from submitted data
// Owned by SceneRenderer - this is graphics-specific
class MeshRenderer
{
public:
    MeshRenderer() = default;
    ~MeshRenderer() = default;

    // Rendering coordination - takes renderable data and generates render commands
    // This is graphics logic, no dependency on any specific data storage
    void Render(const std::vector<RenderableData>& renderables, Camera& camera);

private:
    void GenerateDrawCommand(const RenderableData& renderable, 
                           const glm::mat4& viewMatrix, 
                           const glm::mat4& projectionMatrix, 
                           const glm::vec3& cameraPosition);
};