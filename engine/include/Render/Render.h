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
#include <rsc-core/rp.hpp>
#include "Manager/ResourceSystem.hpp"
#include <native/native.h>

#include "Ecs/ecs.h"

// Forward declarations
class ShaderLibrary;
class PrimitiveManager;
class ComponentInitializer;
class MaterialInstanceManager;
class MaterialInstance;
class MaterialPropertyBlock;


RegisterResourceType(MeshResourceData, "mesh", MeshResourceData, [](MeshResourceData const& mr) {return mr; }, [](MeshResourceData&& mr) {})
RegisterResourceType(MaterialResourceData, "material", MaterialResourceData, [](MaterialResourceData const& mat) {return mat; }, [](MaterialResourceData&& mres) {})

/**
 * @brief Component for rendering meshes on entities
 *
 * Supports both primitive meshes (cube, plane) and asset-based meshes loaded from files.
 * Material properties can be stored per-entity for customization.
 */
struct MeshRendererComponent {
    bool isPrimitive = false;              ///< True if using a primitive mesh (cube/plane)
    bool hasAttachedMaterial = false;      ///< True if using an external material asset

    /// Primitive mesh types available
    enum struct PrimitiveType : std::uint8_t {
        NONE,
        CUBE,
        PLANE
	} m_PrimitiveType{PrimitiveType::NONE};
    rp::TypeNameGuid<"mesh"> m_MeshGuid;     ///< GUID of the mesh asset (or zero for primitives)
    rp::TypeNameGuid<"material"> m_MaterialGuid; ///< GUID of the material asset

    /// Per-entity material properties (used when hasAttachedMaterial is false)
    struct Material
    {
        rp::TypeNameGuid<"material"> m_MaterialGuid;
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
 *
 * Aligned with graphics library's SubmittedLightData structure.
 * All fields are stored regardless of light type (Unity-style approach).
 *
 * USAGE GUIDE:
 * - Directional: Use Direction only (infinite range)
 * - Point: Use Range only (position from Transform)
 * - Spot: Use Direction, Range, InnerCone, OuterCone
 */
struct LightComponent {
    Light::Type m_Type = Light::Type::Directional;           ///< Light type (Directional/Point/Spotlight)

    // === Common Properties (All Types) ===
    glm::vec3 m_Color = glm::vec3(1.0f, 1.0f, 1.0f);         ///< Light color (RGB)
    float m_DiffuseIntensity = 1.0f;                         ///< Direct light intensity (PBR)
    float m_AmbientIntensity = 0.0f;                         ///< Ambient contribution (PBR)
    bool m_IsEnabled = true;                                 ///< Enable/disable light

    // === Shadow Settings (Unity-style) ===
    bool m_CastShadows = true;                               ///< Enable shadow casting

    // === Directional & Spot Only ===
    glm::vec3 m_Direction = glm::vec3(0.0f, -1.0f, 0.0f);    ///< [Dir/Spot] Light direction

    // === Point & Spot Only ===
    float m_Range = 10.0f;                                   ///< [Point/Spot] Max distance

    // === Spot Only ===
    float m_InnerCone = 30.0f;                               ///< [Spot] Inner angle (deg)
    float m_OuterCone = 45.0f;                               ///< [Spot] Outer angle (deg)
};

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
     * - ComponentInitializer (ready for observers)
     * - MaterialInstanceManager (material instance caching)
     */
    RenderSystem();

    /**
     * @brief Destroy RenderSystem and cleanup subsystems
     *
     * Destruction order ensures no dangling dependencies:
     * 1. ComponentInitializer
     * 2. MaterialInstanceManager
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
     * @note Must call SetupComponentObservers() after world is created/loaded
     */
    void Init();

    /**
     * @brief Disable HDR pipeline for editor compatibility
     *
     * When running inside the editor, HDR/tone mapping causes sRGB/gamma issues
     * with ImGui rendering. This method disables the HDR pipeline to use linear
     * RGB16F buffers directly, avoiding gamma correction conflicts.
     *
     * @note Only call this when running from the editor, not for standalone builds
     */
    void DisableHDRForEditor();

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
     * @brief Setup component observers for automatic cleanup
     *
     * Registers observers that clean up rendering resources
     * when MeshRendererComponent is removed from an entity.
     *
     * @param world The ECS world to attach observers to
     */
    void SetupComponentObservers(ecs::world& world);

    // ========== Static API for External Access ==========

    /**
     * @brief Register an editor-created mesh for rendering (in-memory, no file backing)
     *
     * Allows the editor to register meshes created at runtime (e.g., imported models,
     * procedural geometry) before they are saved to disk. The mesh is immediately
     * available for rendering via its GUID.
     *
     * @param guid GUID to associate with the mesh (editor-assigned)
     * @param mesh Shared pointer to the mesh resource
     * @return true if registered successfully, false if GUID already exists
     *
     * @note When the asset is saved, it transitions from in-memory to file-based
     *       automatically (GUID remains the same)
     */
    static void RegisterEditorMesh(rp::Guid guid, std::shared_ptr<Mesh> mesh);

    /**
     * @brief Register an editor-created material for rendering (in-memory, no file backing)
     *
     * Allows the editor to register materials created at runtime before they are
     * saved to disk. The material is immediately available for rendering via its GUID.
     *
     * @param guid GUID to associate with the material (editor-assigned)
     * @param material Shared pointer to the material resource
     * @return true if registered successfully, false if GUID already exists
     *
     * @note When the asset is saved, it transitions from in-memory to file-based
     *       automatically (GUID remains the same)
     */
    static void RegisterEditorMaterial(rp::Guid guid, std::shared_ptr<Material> material);


    /**
     * @brief Update cached material properties from MeshRendererComponent
     *
     * Call this after modifying MeshRendererComponent.material properties to sync
     * the changes to the cached material used for rendering.
     *
     * @param entityUID Unique identifier of the entity
     * @param meshRenderer The updated MeshRendererComponent
     *
     * @note This is automatically called if you use registry.patch<MeshRendererComponent>()
     */
    static void SyncMaterialFromComponent(uint64_t entityUID, const MeshRendererComponent& meshRenderer);

    // ========== Material Instance API ==========

    /**
     * @brief Get or create a material instance for an entity
     *
     * First call creates an instance, subsequent calls return the existing instance.
     * This implements Unity's Renderer.material behavior (copy-on-write).
     *
     * @param entityUID Unique identifier for the entity
     * @param baseMaterial The base material to instance from
     * @return Material instance (never null if baseMaterial is valid)
     */
    std::shared_ptr<MaterialInstance> GetMaterialInstance(uint64_t entityUID,
                                                           std::shared_ptr<Material> baseMaterial);

    /**
     * @brief Check if an entity has a material instance
     * @param entityUID Unique identifier for the entity
     */
    bool HasMaterialInstance(uint64_t entityUID) const;

    /**
     * @brief Get existing material instance (returns nullptr if no instance exists)
     * @param entityUID Unique identifier for the entity
     */
    std::shared_ptr<MaterialInstance> GetExistingMaterialInstance(uint64_t entityUID) const;

    /**
     * @brief Destroy the material instance for an entity
     *
     * After this call, the entity will use the shared material again.
     *
     * @param entityUID Unique identifier for the entity
     */
    void DestroyMaterialInstance(uint64_t entityUID);

    // ========== Material Property Block Management ==========

    /**
     * @brief Get or create a MaterialPropertyBlock for an entity
     *
     * MaterialPropertyBlocks provide lightweight per-object material customization
     * without creating full material instances. They preserve GPU instancing compatibility
     * and use minimal memory (only stores overridden properties).
     *
     * Use Case Comparison:
     * - MaterialPropertyBlock: Small tweaks (color tint, roughness), preserves instancing
     * - MaterialInstance: Full material customization, breaks instancing
     *
     * @param entityUID Unique identifier for the entity
     * @return Property block for the entity (never null, creates if doesn't exist)
     *
     * @code
     * // Example: Red tint on specific entity
     * auto propBlock = renderSystem.GetPropertyBlock(entityUID);
     * propBlock->SetVec3("u_AlbedoColor", glm::vec3(1.0f, 0.0f, 0.0f));
     * propBlock->SetFloat("u_Roughness", 0.8f);
     * @endcode
     */
    std::shared_ptr<MaterialPropertyBlock> GetPropertyBlock(uint64_t entityUID);

    /**
     * @brief Check if an entity has a property block with properties set
     * @param entityUID Unique identifier for the entity
     * @return true if property block exists and has properties
     */
    bool HasPropertyBlock(uint64_t entityUID) const;

    /**
     * @brief Clear all properties from an entity's property block
     *
     * The property block object remains but all properties are cleared.
     *
     * @param entityUID Unique identifier for the entity
     */
    void ClearPropertyBlock(uint64_t entityUID);

    /**
     * @brief Destroy the property block for an entity
     *
     * After this call, the entity will have no property overrides.
     *
     * @param entityUID Unique identifier for the entity
     */
    void DestroyPropertyBlock(uint64_t entityUID);

    // ========== Public Member Access ==========

    std::unique_ptr<SceneRenderer> m_SceneRenderer;  ///< Rendering pipeline interface
    std::unique_ptr<Camera> m_Camera;                ///< Fallback camera (used if no CameraComponent exists)

private:
    // ========== Internal Methods ==========

    /**
     * @brief Handle MeshRendererComponent updates (sync material properties)
     *
     * Called automatically when registry.patch<MeshRendererComponent>() is used.
     *
     * @param registry EnTT registry containing the entity
     * @param entity Entity handle with MeshRendererComponent
     */
    void OnMeshRendererUpdated(entt::registry& registry, entt::entity entity);

    /**
     * @brief Helper to load mesh resource from GUID or primitive type
     * @param meshComp MeshRendererComponent containing resource info
     * @return Mesh pointer, or nullptr if not found
     */
    std::shared_ptr<Mesh> LoadMeshResource(const MeshRendererComponent& meshComp) const;

    /**
     * @brief Helper to load material resource from GUID or create default
     * @param materialGuid Material GUID to load
     * @param hasAttachedMaterial Whether the component has an attached material
     * @param entityUID Entity ID for default material naming
     * @return Material pointer, or nullptr if failed
     */
    std::shared_ptr<Material> LoadMaterialResource(const Resource::Guid& materialGuid, bool hasAttachedMaterial, uint64_t entityUID) const;

    // ========== Render Subsystems ==========

    std::unique_ptr<ShaderLibrary> m_ShaderLibrary;             ///< Shader loading and caching
    std::unique_ptr<PrimitiveManager> m_PrimitiveManager;       ///< Primitive mesh generation
    std::unique_ptr<ComponentInitializer> m_ComponentInitializer; ///< Component lifecycle management
    std::unique_ptr<MaterialInstanceManager> m_MaterialInstanceManager; ///< Material instance management

    // ========== Material Property Blocks ==========

    /// Per-entity property blocks for lightweight material customization
    /// Key: Entity UID, Value: Property block
    std::unordered_map<uint64_t, std::shared_ptr<MaterialPropertyBlock>> m_PropertyBlocks;
};

#endif