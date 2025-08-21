// Engine/ECS/Systems/RenderSystem.h
#pragma once

#include <entt/entt.hpp>
#include "../../Scene/Scene.h"
#include "../../Core/Renderer.h"

class RenderSystem
{
public:
    void OnUpdate(Scene* scene);

private:
    void DrawEntity(entt::registry& registry, entt::entity entity);
};