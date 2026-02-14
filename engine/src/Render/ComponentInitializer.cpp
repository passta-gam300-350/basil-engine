/******************************************************************************/
/*!
\file   ComponentInitializer.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Component initializer implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Render/ComponentInitializer.hpp"
#include "Render/Render.h"
#include "Engine.hpp"
#include <spdlog/spdlog.h>

void ComponentInitializer::SetupObservers(ecs::world& world) {
    entt::registry& registry = world.impl.get_registry();

    // Disconnect any existing observers first (in case this is called multiple times)
    registry.on_destroy<MeshRendererComponent>().disconnect();

    // Set up observer for when MeshRendererComponent is destroyed
    registry.on_destroy<MeshRendererComponent>().connect<&ComponentInitializer::OnMeshRendererDestroyed>(this);

    spdlog::info("ComponentInitializer: Component observer registered (destroy only)");
}

void ComponentInitializer::OnMeshRendererDestroyed(entt::registry& /*registry*/, entt::entity entity) {
    // Convert entt::entity to entity ID
    const uint64_t entityUID = static_cast<uint64_t>(ecs::world::detail::entity_id_cast(entity));

    spdlog::debug("ComponentInitializer: Cleaning up resources for destroyed entity {}", entityUID);

    // Clean up material instances and property blocks via RenderSystem
    auto& renderSystem = Engine::GetRenderSystem();

    // Destroy material instance if it exists
    if (renderSystem.HasMaterialInstance(entityUID)) {
        renderSystem.DestroyMaterialInstance(entityUID);
    }

    // Destroy property block if it exists
    if (renderSystem.HasPropertyBlock(entityUID)) {
        renderSystem.DestroyPropertyBlock(entityUID);
    }

    spdlog::debug("ComponentInitializer: Successfully cleaned up all resources for entity {}", entityUID);
}
