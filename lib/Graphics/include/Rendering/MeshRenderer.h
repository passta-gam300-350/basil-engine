#pragma once

#include "../../../test/examples/lib/Graphics/Engine/Scene/Scene.h"
#include "../Utility/Camera.h"
#include <memory>

// Rendering coordinator - queries ECS for renderable entities and generates commands
// Owned by SceneRenderer, not Scene - this is graphics-specific
class MeshRenderer
{
public:
    MeshRenderer() = default;
    ~MeshRenderer() = default;

    // Rendering coordination - queries scene and generates render commands
    // This is graphics logic, not ECS logic
    void Render(Scene* scene, Camera& camera);

private:
    void RenderVisibleMeshes(Scene* scene, Camera& camera);
    void GenerateDrawCommand(entt::registry& registry, entt::entity entity, 
                           const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, 
                           Camera& camera);
};