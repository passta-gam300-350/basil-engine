#pragma once

#include <entt/entt.hpp>
#include "../../Scene/Scene.h"
#include "../../Core/Renderer.h"
#include "../../Utility/Camera.h"
#include <glm/glm.hpp>

class RenderSystem
{
public:
    void OnUpdate(Scene* scene);
    void OnUpdate(Scene* scene, Camera& camera);  // Overload that takes camera directly

private:
    void DrawEntity(entt::registry& registry, entt::entity entity, Camera* camera, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
};