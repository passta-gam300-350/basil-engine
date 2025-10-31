#ifndef ENGINE_COMPONENT_INITIALIZER_HPP
#define ENGINE_COMPONENT_INITIALIZER_HPP
#pragma once

#include "Ecs/ecs.h"
#include <entt/entt.hpp>

// Forward declarations
class RenderResourceCache;
class ShaderLibrary;
class PrimitiveManager;

/**
 * @brief Handles component initialization and observer setup for the render system
 *
 * Responsibilities:
 * - Initialize MeshRenderer components (load meshes, create materials)
 * - Set up EnTT observers for component lifecycle events
 * - Batch initialization of existing entities
 *
 * Separates initialization logic from rendering logic for better maintainability.
 * This class does NOT own the subsystems - it only holds references to them.
 *
 * Lifetime: Owned by RenderSystem
 */
class ComponentInitializer {
public:
    /**
     * @brief Construct a ComponentInitializer with references to render subsystems
     * @param cache Reference to resource cache for mesh/material storage
     * @param shaderLibrary Reference to shader library for material creation
     * @param primitiveManager Reference to primitive manager for primitive mesh creation
     */
    ComponentInitializer(
        RenderResourceCache& cache,
        ShaderLibrary& shaderLibrary,
        PrimitiveManager& primitiveManager
    );

    ~ComponentInitializer() = default;

    // Delete copy/move to ensure references remain valid
    ComponentInitializer(const ComponentInitializer&) = delete;
    ComponentInitializer& operator=(const ComponentInitializer&) = delete;
    ComponentInitializer(ComponentInitializer&&) = delete;
    ComponentInitializer& operator=(ComponentInitializer&&) = delete;

    /**
     * @brief Initialize a single entity's MeshRenderer component
     * @param registry EnTT registry containing the entity
     * @param entity Entity to initialize
     *
     * Called automatically by EnTT observers when MeshRendererComponent is added,
     * or manually for existing entities.
     */
    void Initialize(entt::registry& registry, entt::entity entity);

    /**
     * @brief Initialize all existing entities with MeshRendererComponent
     * @param world ECS world to scan for entities
     *
     * Should be called once after world load and after SetupObservers().
     */
    void InitializeAll(ecs::world& world);

    /**
     * @brief Set up EnTT observers for automatic component initialization
     * @param world ECS world to attach observers to
     *
     * Should be called once after RenderSystem initialization and before InitializeAll().
     */
    void SetupObservers(ecs::world& world);

private:
    /**
     * @brief Cleanup callback when MeshRendererComponent is destroyed
     * @param registry EnTT registry containing the entity
     * @param entity Entity being destroyed
     *
     * Called automatically by EnTT observers when MeshRendererComponent is removed.
     * Cleans up cached rendering resources (meshes, materials, material instances).
     */
    void OnMeshRendererDestroyed(entt::registry& registry, entt::entity entity);

    // References to render subsystems (NOT owned by this class)
    RenderResourceCache& m_ResourceCache;
    ShaderLibrary& m_ShaderLibrary;
    PrimitiveManager& m_PrimitiveManager;
};

#endif // ENGINE_COMPONENT_INITIALIZER_HPP
