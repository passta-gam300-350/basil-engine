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

#include <memory>
#include <Scene/SceneRenderer.h>
#include <Utility/Camera.h>
#include <rsc-core/rp.hpp>
#include "Manager/ResourceSystem.hpp"
#include <native/native.h>
#include "Ecs/ecs.h"
#include "Messaging/Message.h"
#include "BVH/bvh.h"
#include "BVH/bvh_renderable.h"
#include "BVH/bvh_helper.h"

// Forward declarations
class ShaderLibrary;
class PrimitiveManager;
class ComponentInitializer;
class MaterialInstanceManager;
class MaterialInstance;
class MaterialPropertyBlock;
class JoltDebugRenderer;


RegisterResourceTypeForward(MeshResourceData, "mesh", meshdefine)
RegisterResourceTypeForward(MaterialResourceData, "material", materialdefine)
RegisterResourceTypeForward(TextureResourceData, "texture", texturedefine)
RegisterResourceTypeForward(FontAtlasResourceData, "font_atlas", fontatlasdefine)

/**
 * @brief Component for rendering meshes on entities
 *
 * Supports both primitive meshes (cube, plane) and asset-based meshes loaded from files.
 * Material properties can be stored per-entity for customization.
 */
struct MeshRendererComponent {
    bool isPrimitive;              ///< True if using a primitive mesh (cube/plane)
    bool hasAttachedMaterial;      ///< True if using an external material asset

    // Primitive mesh types available
    enum struct PrimitiveType : std::uint8_t {
        NONE,
        CUBE,
        PLANE
	} m_PrimitiveType;
    rp::BasicIndexedGuid m_MeshGuid{ static_cast<rp::BasicIndexedGuid>(rp::TypeNameGuid<"mesh">{}) };     ///< GUID of the mesh asset (or zero for primitives)
    std::unordered_map<std::string, rp::BasicIndexedGuid> m_MaterialGuid{ std::pair<std::string, rp::BasicIndexedGuid>("unnamed slot", static_cast<rp::BasicIndexedGuid>(rp::TypeNameGuid<"material">{}))}; ///< GUID of the material asset

    // Per-entity material properties (used when hasAttachedMaterial is false)
    struct Material
    {
        rp::BasicIndexedGuid m_MaterialGuid;
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
    bool m_CastShadows = true;    ///< Enable/disable shadow casting for this light
};

/**
 * @brief Component for rendering HUD/UI elements in screen space
 *
 * HUD elements are rendered in pixel coordinates after all 3D rendering and post-processing.
 * Supports textured quads and solid color rectangles with rotation and layering.
 */
struct HUDComponent {
    rp::BasicIndexedGuid m_TextureGuid{ static_cast<rp::BasicIndexedGuid>(rp::TypeNameGuid<"texture">{}) };  ///< GUID of the texture asset (0 = solid color quad)

    // Screen-space positioning (pixels)
    glm::vec2 position = glm::vec2(0.0f);  ///< Position in pixels from anchor point
    glm::vec2 size = glm::vec2(100.0f);    ///< Size in pixels

    /**
     * @brief Anchor point for positioning
     *
     * Determines which point of the element corresponds to the specified position.
     * For example, TopLeft means position=(0,0) places the top-left corner at screen origin.
     */
    enum class Anchor : uint8_t {
        TopLeft,
        TopCenter,
        TopRight,
        CenterLeft,
        Center,
        CenterRight,
        BottomLeft,
        BottomCenter,
        BottomRight
    } anchor = Anchor::TopLeft;

    // Visual properties
    glm::vec4 color = glm::vec4(1.0f);  ///< Tint color (multiplied with texture) or solid color
    float rotation = 0.0f;              ///< Rotation in degrees (clockwise, around anchor point)
    uint8_t layer = 0;                 ///< Layer for depth sorting (higher values render on top)
    bool visible = true;                ///< Visibility toggle
};

/**
 * @brief Component for rendering SDF text in screen space
 *
 * Text elements are rendered using signed distance field (SDF) fonts for crisp, scalable text.
 * Supports outline, glow, and shadow effects, as well as rich text formatting.
 */
struct TextComponent {
    rp::BasicIndexedGuid m_FontGuid{ static_cast<rp::BasicIndexedGuid>(rp::TypeNameGuid<"font_atlas">{}) };  ///< GUID of the font atlas asset

    // Text content
    std::string text = "Text";  ///< Text string (UTF-8 encoded)

    // Screen-space positioning (pixels)
    glm::vec2 position = glm::vec2(0.0f);  ///< Position in pixels from anchor point
    float fontSize = 16.0f;                ///< Font size in pixels

    /**
     * @brief Anchor point for positioning (reuses HUDComponent::Anchor)
     */
    enum class Anchor : uint8_t {
        TopLeft,
        TopCenter,
        TopRight,
        CenterLeft,
        Center,
        CenterRight,
        BottomLeft,
        BottomCenter,
        BottomRight
    } anchor = Anchor::TopLeft;

    /**
     * @brief Text alignment for multi-line text
     */
    enum class Alignment : uint8_t {
        Left,
        Center,
        Right,
        Justified
    } alignment = Alignment::Left;

    // Layout properties
    float lineSpacing = 1.0f;     ///< Line spacing multiplier (1.0 = default line height)
    float letterSpacing = 0.0f;   ///< Letter spacing in pixels
    float maxWidth = 0.0f;        ///< Maximum width for word wrapping (0 = no wrapping)

    // Visual properties
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  ///< Text color (RGBA)

    // Outline effect
    float outlineWidth = 0.0f;                              ///< Outline thickness (0.0-0.5, 0 = no outline)
    glm::vec4 outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); ///< Outline color (RGBA)

    // Glow/shadow effect
    float glowStrength = 0.0f;                              ///< Glow/shadow strength (0.0-0.5, 0 = no glow)
    glm::vec4 glowColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.5f); ///< Glow/shadow color (RGBA)

    // SDF rendering parameters
    float sdfThreshold = 0.5f;    ///< SDF cutoff threshold (0.5 = sharp edges)
    float smoothing = 0.04f;      ///< Edge smoothing factor (anti-aliasing)

    // Transform
    float rotation = 0.0f;        ///< Rotation in degrees (clockwise, around anchor point)
    uint8_t layer = 0;           ///< Layer for depth sorting (higher values render on top)
    bool visible = true;          ///< Visibility toggle
};


struct Camera_Calculation_Update : Message {
    glm::mat4 viewMat4;
    glm::mat4 projectionMat4;

public:
    /**
    * @brief Clone function for duplicating input message.
    *
    * This function provides a way to clone unique_ptrs.
    */
    std::unique_ptr<Message> clone() const override
    {
        return std::make_unique<Camera_Calculation_Update>(*this);
    }
    Camera_Calculation_Update(glm::mat4 view, glm::mat4 proj) : viewMat4(view), projectionMat4(proj) {};

    ~Camera_Calculation_Update() = default;
};

/**
 * @brief Component marking an entity as interactable for proximity-based interaction
 *
 * Used for gameplay mechanics where players can interact with objects when looking at them.
 * Detection uses ray casting from camera forward direction.
 * Only entities with this component will be detected by QueryInteraction().
 */
struct InteractableComponent {
    bool m_IsEnabled = true;                     ///< Can be toggled on/off at runtime
    float m_InteractionDistance = 5.0f;          ///< Maximum distance for interaction (meters)
    std::string m_InteractionPrompt = "Press E to interact";  ///< UI text to display
};

/**
 * @brief Result from interaction detection query
 *
 * Used for proximity-based interaction detection (e.g., "Press E to interact" prompts).
 * Returned by QueryInteraction() to indicate if player is looking at an interactable object.
 */
struct InteractionResult {
    bool hasHit = false;              ///< Was an object detected?
    uint32_t entityUID = 0;           ///< ID of the detected entity
    float distance = 0.0f;            ///< Distance from camera to object (meters)
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

    void DisableHDRForEditor();

    // ========== Viewport Management ==========

    /**
     * @brief Set Editor (Scene) viewport dimensions
     *
     * Called by EngineService when the Scene viewport is resized.
     * Used for correct HUD rendering and framebuffer sizing.
     *
     * @param width Scene viewport width in pixels
     * @param height Scene viewport height in pixels
     */
    void SetEditorViewportSize(uint32_t width, uint32_t height) {
        m_editorViewportWidth = width;
        m_editorViewportHeight = height;
    }

    /**
     * @brief Set Game viewport dimensions
     *
     * Called by EngineService when the Game viewport is resized.
     * Used for correct HUD rendering and framebuffer sizing.
     *
     * @param width Game viewport width in pixels
     * @param height Game viewport height in pixels
     */
    void SetGameViewportSize(uint32_t width, uint32_t height) {
        m_gameViewportWidth = width;
        m_gameViewportHeight = height;
    }

    // ========== System Setup Methods ==========

    /**
     * @brief Setup debug visualization meshes
     *
     * Creates:
     * - Light visualization cube (5x5x5)
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

    // ========== Public Accessors ==========

    /**
     * @brief Get the SceneRenderer instance
     * @return Pointer to SceneRenderer (may be nullptr if not initialized)
     */
    SceneRenderer* GetSceneRenderer() const { return m_SceneRenderer.get(); }

    /**
     * @brief Get the ShaderLibrary instance
     * @return Pointer to ShaderLibrary (may be nullptr if not initialized)
     */
    ShaderLibrary* GetShaderLibrary() const { return m_ShaderLibrary.get(); }

    /**
     * @brief Get the Jolt debug renderer instance
     * @return Pointer to JoltDebugRenderer (may be nullptr if not initialized)
     */
    JoltDebugRenderer* GetJoltDebugRenderer() const { return m_JoltDebugRenderer.get(); }

    /**
     * @brief Enable or disable Jolt physics debug rendering
     * @param enabled True to enable debug rendering, false to disable
     */
    void SetJoltDebugRenderingEnabled(bool enabled);

    /**
     * @brief Initialize Jolt debug renderer (must be called after PhysicsSystem::Init)
     * @note This is called automatically during engine initialization
     */
    void InitJoltDebugRenderer();

    // ========== Skybox Settings (Unity-style API) ==========

    /**
     * @brief Set skybox cubemap texture
     * @param cubemapID OpenGL texture ID of the cubemap (load via TextureLoader::CubemapFromFiles)
     */
    void SetSkyboxCubemap(unsigned int cubemapID);

    /**
     * @brief Enable or disable skybox rendering
     * @param enable True to enable, false to disable
     */
    void EnableSkybox(bool enable);

    /**
     * @brief Check if skybox is currently enabled
     * @return True if enabled, false otherwise
     */
    bool IsSkyboxEnabled() const;

    /**
     * @brief Set skybox HDR exposure multiplier
     * @param exposure Exposure value (0.0 - 10.0), default is 1.0
     */
    void SetSkyboxExposure(float exposure);

    /**
     * @brief Set skybox rotation (Euler angles in degrees)
     * @param rotation XYZ rotation in degrees
     */
    void SetSkyboxRotation(const glm::vec3& rotation);

    /**
     * @brief Set skybox color tint
     * @param tint RGB color multiplier (0.0 - 1.0), default is white (1, 1, 1)
     */
    void SetSkyboxTint(const glm::vec3& tint);

    /**
     * @brief Load a cubemap from 6 image files and return the OpenGL texture ID
     * @param facePaths Array of 6 file paths in order: +X, -X, +Y, -Y, +Z, -Z (right, left, top, bottom, front, back)
     * @param directory Base directory for the face paths (optional)
     * @return OpenGL texture ID, or 0 on failure
     */
    static unsigned int LoadCubemapFromFiles(
        const std::array<std::string, 6>& facePaths,
        const std::string& directory = ""
    );

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
    static bool RegisterEditorMesh(rp::Guid guid, std::shared_ptr<Mesh> mesh);

    /**
     * @brief Register an editor-created material for use in rendering
     *
     * @param guid Unique identifier for the material
     * @param material Shared pointer to the material resource
     *
     * @note Delegates to Engine::GetRenderSystem() for backward compatibility
     */
    static bool RegisterEditorMaterial(rp::Guid guid, std::shared_ptr<Material> material);

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

    /**
     * @brief Build spatial index for all renderable entities
     * Clears existing BVH and rebuilds from scratch.
     * Should be called automatically during InitializeExistingEntities().
     * @param world The ECS world containing entities
     */
    void BuildBVH(ecs::world& world);

    /**
     * @brief Build spatial index for interactable entities only
     * Filters for entities with InteractableComponent.
     * Should be called after scene load.
     * @param world The ECS world containing entities
     */
    void BuildInteractableBVH(ecs::world& world);

    // ========== Public Member Access ==========

    std::unique_ptr<SceneRenderer> m_SceneRenderer;  ///< Rendering pipeline interface

    // ========== BVH for Frustum Culling ==========
    std::unordered_map<uint64_t, std::unique_ptr<BvhRenderable>> m_BvhRenderables;
    Bvh<BvhRenderable*> m_bvh;
    BvhBuildConfig m_BvhConfig;
    bool m_frustumCullingEnabled = false;

    // ========== BVH for Interactables (Gameplay) ==========
    std::unordered_map<uint64_t, std::unique_ptr<BvhRenderable>> m_BvhInteractables;
    Bvh<BvhRenderable*> m_bvhInteractables;

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

    /**
     * @brief Handle MeshRendererComponent updates (sync material properties)
     *
     * Called automatically when registry.patch<MeshRendererComponent>() is used.
     *
     * @param registry EnTT registry containing the entity
     * @param entity Entity handle with MeshRendererComponent
     */
    void OnMeshRendererUpdated(entt::registry& registry, entt::entity entity);

    // ========== Resource Loading Helpers ==========

    /**
     * @brief Load mesh resource from ResourceRegistry or fallback to primitive
     * @param meshComp MeshRendererComponent containing mesh GUID
     * @return Loaded mesh or primitive fallback
     */
    std::variant<std::shared_ptr<Mesh>, std::vector<std::pair<std::string, std::shared_ptr<Mesh>>>*> LoadMeshResource(const MeshRendererComponent& meshComp) const;

    /**
     * @brief Load material resource from ResourceRegistry or create default
     * @param materialGuid GUID of material to load
     * @param hasAttachedMaterial Whether entity has attached material
     * @param entityUID Entity UID for fallback material naming
     * @return Loaded material or default/fallback material
     */
    std::shared_ptr<Material> LoadMaterialResource(
        const rp::TypeNameGuid<"material">& materialGuid,
        bool hasAttachedMaterial,
        uint64_t entityUID) const;

    /**
     * @brief Compute world-space AABB for an entity
     * @param entity Entity with MeshRendererComponent and TransformMtxComponent
     * @return World-space AABB
     */
    Aabb ComputeWorldAABB(ecs::entity entity) const;

    /**
     * @brief Perform frustum culling to get visible entity IDs
     * Uses the BVH spatial index to query which entities are within the camera frustum.
     * Falls back to returning all entities if culling is disabled or BVH is empty.
     * @param world The ECS world containing entities
     * @param camera The camera to build frustum from (editor or entity camera)
     * @return Vector of entity UIDs that are potentially visible
     */
    std::vector<unsigned> GetVisibleEntities(ecs::world& world, auto const& camera) const;

    /**
     * @brief Query for object in front of camera using ray casting
     *
     * Uses BVH ray casting to detect which entity the camera is looking at.
     * Useful for proximity-based interaction detection (e.g., "Press E to interact").
     *
     * @param world The ECS world containing entities
     * @param camera The camera to cast ray from (typically game entity camera)
     * @return InteractionResult with detected entity info (hasHit=false if nothing detected)
     */
    //InteractionResult QueryInteraction(ecs::world& world, auto const& camera) const;

    // ========== Render Subsystems ==========

    std::unique_ptr<ShaderLibrary> m_ShaderLibrary;             ///< Shader loading and caching
    std::unique_ptr<PrimitiveManager> m_PrimitiveManager;       ///< Primitive mesh generation
    std::unique_ptr<ComponentInitializer> m_ComponentInitializer; ///< Component initialization logic
    std::unique_ptr<MaterialInstanceManager> m_MaterialInstanceManager; ///< Material instance management
    std::unique_ptr<JoltDebugRenderer> m_JoltDebugRenderer;     ///< Jolt physics debug renderer

    // ========== Material Property Blocks ==========

    /// Per-entity property blocks for lightweight material customization
    /// Key: Entity UID, Value: Property block
    std::unordered_map<uint64_t, std::shared_ptr<MaterialPropertyBlock>> m_PropertyBlocks;

    // ========== Default Material Cache ==========

    /// Cached default material for entities without attached materials
    /// Shared across all entities with hasAttachedMaterial=false
    /// Ensures stable memory address to prevent cache thrashing
    mutable std::shared_ptr<Material> m_DefaultMaterial;

    // ========== Viewport Dimensions ==========

    /// Editor (Scene) viewport dimensions for framebuffer sizing
    uint32_t m_editorViewportWidth{ 0 };
    uint32_t m_editorViewportHeight{ 0 };

    /// Game viewport dimensions for framebuffer sizing
    uint32_t m_gameViewportWidth{ 0 };
    uint32_t m_gameViewportHeight{ 0 };

    // ========== Editor Camera Snapshot (Phase 2: Thread-safe camera data) ==========

public:
    /// Editor camera snapshot data (set by EngineService, read by Update)
    struct EditorCameraSnapshot {
        glm::vec3 position{ 0.0f, 5.0f, 10.0f };
        glm::vec3 front{ 0.0f, 0.0f, -1.0f };
        glm::vec3 up{ 0.0f, 1.0f, 0.0f };
        glm::vec3 right{ 1.0f, 0.0f, 0.0f };
        float fov{ 45.0f };
        float aspectRatio{ 16.0f / 9.0f };
        float nearPlane{ 0.1f };
        float farPlane{ 1000.0f };
        bool isPerspective{ true };
    };

    /**
     * @brief Set editor camera snapshot (called by EngineService on engine thread)
     * @param snapshot Camera data from EditorMain (synchronized via mutex)
     */
    void SetEditorCameraSnapshot(const EditorCameraSnapshot& snapshot) {
        m_editorCameraSnapshot = snapshot;
    }
    EditorCameraSnapshot GetEditorCameraSnapshot() const {
        return m_editorCameraSnapshot;
    }

private:
    EditorCameraSnapshot m_editorCameraSnapshot;
};

#endif