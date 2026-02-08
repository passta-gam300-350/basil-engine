/******************************************************************************/
/*!
\file   ComponentInitializer.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Component initialization for rendering

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef ENGINE_COMPONENT_INITIALIZER_HPP
#define ENGINE_COMPONENT_INITIALIZER_HPP
#pragma once

#include "Ecs/ecs.h"
#include <entt/entt.hpp>

/**
 * @brief Handles component lifecycle observers for the render system
 *
 * Responsibilities:
 * - Set up EnTT observers for component lifecycle events
 * - Clean up rendering resources when components are destroyed
 *
 * Resources are now loaded on-demand via ResourceRegistry in RenderSystem::Update(),
 * so this class primarily handles cleanup.
 *
 * Lifetime: Owned by RenderSystem
 */
class ComponentInitializer {
public:
    /**
     * @brief Default constructor
     */
    ComponentInitializer() = default;
    ~ComponentInitializer() = default;

    // Delete copy/move to prevent accidental duplication
    ComponentInitializer(const ComponentInitializer&) = delete;
    ComponentInitializer& operator=(const ComponentInitializer&) = delete;
    ComponentInitializer(ComponentInitializer&&) = delete;
    ComponentInitializer& operator=(ComponentInitializer&&) = delete;

    /**
     * @brief Set up EnTT observers for automatic cleanup
     * @param world ECS world to attach observers to
     *
     * Should be called once after RenderSystem initialization.
     */
    void SetupObservers(ecs::world& world);

private:
    /**
     * @brief Cleanup callback when MeshRendererComponent is destroyed
     * @param registry EnTT registry containing the entity
     * @param entity Entity being destroyed
     *
     * Called automatically by EnTT observers when MeshRendererComponent is removed.
     * Cleans up material instances and property blocks.
     */
    void OnMeshRendererDestroyed(entt::registry& registry, entt::entity entity);
};

#endif // ENGINE_COMPONENT_INITIALIZER_HPP
