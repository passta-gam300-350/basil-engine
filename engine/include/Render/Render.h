/**
 * @file Render.h
 * @brief RenderSystem and rendering component definitions for the engine
 *
 * This file defines the main RenderSystem class and ECS components used for rendering.
 * The RenderSystem follows a clean architecture with separated concerns:
 * - ShaderLibrary: Manages shader loading and caching
 * - PrimitiveManager: Creates and caches primitive meshes
 * - RenderResourceCache: Caches entity-specific rendering resources
 * - ComponentInitializer: Handles component initialization logic
 *
 * @note After refactoring (Phase 5), RenderSystem is no longer a singleton.
 *       Access via Engine::GetRenderSystem() instead of static methods.
 */

#ifndef ENGINE_RENDER_H
#define ENGINE_RENDER_H
#pragma once

#include <memory>
#include <Scene/SceneRenderer.h>
#include <Utility/Camera.h>
#include <serialisation/guid.h>
#include "Manager/ResourceSystem.hpp"
#include <native/mesh.h>

#include "Ecs/ecs.h"

// Forward declarations
class ShaderLibrary;
class PrimitiveManager;
class RenderResourceCache;
class ComponentInitializer;

/**
 * @brief Component for rendering meshes on entities
 *
 * Supports both primitive meshes (cube, plane) and asset-based meshes loaded from files.
 * Material properties can be stored per-entity for customization.
 */
struct MeshRendererComponent {
    bool isPrimitive;              ///< True if using a primitive mesh (cube/plane)
    bool hasAttachedMaterial;      ///< True if using an external material asset

    /// Primitive mesh types available
    enum struct PrimitiveType : std::uint8_t {
        NONE,
        CUBE,
        PLANE
	} m_PrimitiveType;

    Resource::Guid m_MeshGuid;     ///< GUID of the mesh asset (or zero for primitives)
    Resource::Guid m_MaterialGuid; ///< GUID of the material asset

    /// Per-entity material properties (used when hasAttachedMaterial is false)
    struct Material
    {
        Resource::Guid m_MaterialGuid;
        float metallic;            ///< Metallic value (0.0 = dielectric, 1.0 = metallic)
		float roughness;           ///< Surface roughness (0.0 = smooth, 1.0 = rough)
		glm::vec3 m_AlbedoColor;   ///< Base color (RGB)
    } material;
};

/**
 * @brief Component controlling entity visibility in rendering
 */
struct VisibilityComponent{
    bool m_IsVisible;  ///< If false, entity will not be rendered
};

/**
 * @brief Component for light sources (directional, point, spotlight)
 */
struct LightComponent {
    Light::Type m_Type;           ///< Light type (Directional/Point/Spotlight)
    glm::vec3 m_Direction;        ///< Direction for directional/spotlights
    glm::vec3 m_Color;            ///< Light color (RGB)
    float m_Intensity;            ///< Light intensity multiplier
    float m_Range;                ///< Maximum range for point/spotlights
    float m_InnerCone;            ///< Inner cone angle for spotlights (degrees)
    float m_OuterCone;            ///< Outer cone angle for spotlights (degrees)
    bool m_IsEnabled;             ///< Light enabled state
};

/**
 * @brief Component for camera entities in the scene
 *
 * The RenderSystem reads from the first CameraComponent entity found.
 * Supports both perspective and orthographic projection.
 */
struct CameraComponent {
    enum class CameraType : std::uint8_t {
        ORTHO,
        PERSPECTIVE
    } m_Type;
    bool m_IsActive;              ///< Camera active state
    float m_Fov;                  ///< Field of view (degrees, perspective only)
    float m_AspectRatio;          ///< Aspect ratio (width/height)
    float m_Near;                 ///< Near clipping plane distance
    float m_Far;                  ///< Far clipping plane distance
    glm::vec3 m_Up;               ///< Camera up vector (world space)
    glm::vec3 m_Right;            ///< Camera right vector (world space)
    glm::vec3 m_Front;            ///< Camera forward vector (world space)
    float m_Yaw;                  ///< Yaw rotation (degrees)
    float m_Pitch;                ///< Pitch rotation (degrees)
};

/**
 * @brief Main rendering system for the engine
 *
 * RenderSystem is responsible for:
 * - Managing the graphics rendering pipeline via SceneRenderer
 * - Processing ECS entities with rendering components (MeshRendererComponent, LightComponent, CameraComponent)
 * - Managing render-specific resources (shaders, primitives, materials)
 * - Coordinating component initialization and resource caching
 *
 * @note RenderSystem is NOT a singleton - it's owned by the Engine class
 *       Access via Engine::GetRenderSystem() instead of static Instance() methods
 *
 * @section usage Usage
 * @code
 * // Initialization (done by Engine)
 * auto renderSystem = std::make_unique<RenderSystem>();
 * renderSystem->Init();
 * renderSystem->SetupComponentObservers(world);
 * renderSystem->InitializeExistingEntities(world);
 *
 * // Per-frame update
 * renderSystem->Update(world);
 *
 * // Cleanup (done by Engine)
 * renderSystem->Exit();
 * @endcode
 *
 * @section architecture Architecture
 * RenderSystem delegates responsibilities to specialized subsystems:
 * - ShaderLibrary: Loads and caches shaders
 * - PrimitiveManager: Creates shared primitive meshes (cube, plane, etc.)
 * - RenderResourceCache: Caches entity-specific render resources
 * - ComponentInitializer: Handles MeshRendererComponent initialization logic
 *
 * @see ShaderLibrary, PrimitiveManager, RenderResourceCache, ComponentInitializer
 */
struct RenderSystem : public ecs::SystemBase {
public:
    // ========== Constructor and Destructor ==========

    /**
     * @brief Construct RenderSystem and initialize all subsystems
     *
     * Initializes:
     * - SceneRenderer (rendering pipeline)
     * - Camera (default perspective camera)
     * - ShaderLibrary (loads essential shaders)
     * - PrimitiveManager (creates common primitives)
     * - RenderResourceCache (empty cache)
     * - ComponentInitializer (ready for observers)
     */
    RenderSystem();

    /**
     * @brief Destroy RenderSystem and cleanup subsystems
     *
     * Destruction order ensures no dangling dependencies:
     * 1. ComponentInitializer (depends on other subsystems)
     * 2. RenderResourceCache
     * 3. PrimitiveManager
     * 4. ShaderLibrary
     * 5. SceneRenderer
     * 6. Camera
     */
    ~RenderSystem();

    // Delete copy/move to prevent accidental duplication
    RenderSystem(const RenderSystem&) = delete;
    RenderSystem& operator=(const RenderSystem&) = delete;
    RenderSystem(RenderSystem&&) = delete;
    RenderSystem& operator=(RenderSystem&&) = delete;

    // ========== System Lifecycle ==========

    /**
     * @brief Complete initialization after construction
     *
     * Sets up debug visualization meshes. Called by Engine::Init().
     * @note Must call SetupComponentObservers() and InitializeExistingEntities()
     *       after world is created/loaded
     */
    void Init();

    /**
     * @brief Update rendering system for current frame
     *
     * Processing order:
     * 1. Clear frame buffer
     * 2. Update camera from ECS CameraComponent
     * 3. Process all MeshRendererComponent entities
     * 4. Process all LightComponent entities
     * 5. Submit data to SceneRenderer
     * 6. Execute rendering pipeline
     *
     * @param world The ECS world containing entities to render
     */
    void Update(ecs::world& world);

    /**
     * @brief Fixed timestep update (currently delegates to Update)
     * @param world The ECS world containing entities to render
     */
    void FixedUpdate(ecs::world& world);

    /**
     * @brief Shutdown rendering system
     *
     * Clears all entity caches before destruction.
     */
    void Exit();

    // ========== System Setup Methods ==========

    /**
     * @brief Setup debug visualization meshes
     *
     * Creates:
     * - Light visualization cube (5x5x5)
     * - Directional light ray (3 units)
     * - AABB wireframe cube (1x1x1)
     */
    void SetupDebugVisualization();

    /**
     * @brief Initialize rendering resources for all existing entities
     *
     * Must be called after loading a scene to initialize resources for
     * entities that were created before observers were set up.
     *
     * @param world The ECS world containing existing entities
     */
    void InitializeExistingEntities(ecs::world& world);

    /**
     * @brief Setup component observers for automatic initialization
     *
     * Registers observers that automatically initialize rendering resources
     * when MeshRendererComponent is added to an entity.
     *
     * @param world The ECS world to attach observers to
     */
    void SetupComponentObservers(ecs::world& world);

    // ========== Static API for External Access ==========

    /**
     * @brief Register an editor-created mesh for use in rendering
     *
     * Editor meshes are cached separately from entity meshes to allow
     * reuse across multiple entities.
     *
     * @param guid Unique identifier for the mesh
     * @param mesh Shared pointer to the mesh resource
     *
     * @note Delegates to Engine::GetRenderSystem() for backward compatibility
     */
    static void RegisterEditorMesh(Resource::Guid guid, std::shared_ptr<Mesh> mesh);

    /**
     * @brief Register an editor-created material for use in rendering
     *
     * @param guid Unique identifier for the material
     * @param material Shared pointer to the material resource
     *
     * @note Delegates to Engine::GetRenderSystem() for backward compatibility
     */
    static void RegisterEditorMaterial(Resource::Guid guid, std::shared_ptr<Material> material);

    /**
     * @brief Clear cached rendering resources for a specific entity
     *
     * Should be called when an entity is destroyed or its rendering components change.
     *
     * @param entityUID Unique identifier of the entity
     */
    static void ClearEntityResources(uint64_t entityUID);

    /**
     * @brief Clear all cached entity rendering resources
     *
     * Useful for scene unload or major state transitions.
     */
    static void ClearAllEntityCaches();

    // ========== Public Member Access ==========

    std::unique_ptr<SceneRenderer> m_SceneRenderer;  ///< Rendering pipeline interface
    std::unique_ptr<Camera> m_Camera;                ///< Fallback camera (used if no CameraComponent exists)

private:
    // ========== Internal Methods ==========

    /**
     * @brief Initialize rendering resources for a MeshRendererComponent
     *
     * Called automatically by ComponentInitializer when MeshRendererComponent is added.
     *
     * @param registry EnTT registry containing the entity
     * @param entity Entity handle with MeshRendererComponent
     */
    void InitializeMeshRenderer(entt::registry& registry, entt::entity entity);

    // ========== Render Subsystems ==========

    std::unique_ptr<ShaderLibrary> m_ShaderLibrary;             ///< Shader loading and caching
    std::unique_ptr<PrimitiveManager> m_PrimitiveManager;       ///< Primitive mesh generation
    std::unique_ptr<RenderResourceCache> m_ResourceCache;       ///< Entity resource caching
    std::unique_ptr<ComponentInitializer> m_ComponentInitializer; ///< Component initialization logic
};

#endif