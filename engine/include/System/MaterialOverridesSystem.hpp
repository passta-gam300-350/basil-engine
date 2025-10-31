/******************************************************************************/
/*!
\file   MaterialOverridesSystem.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2025/10/31
\brief  System for managing MaterialOverridesComponent and MaterialInstance lifecycle

        This system bridges the gap between MaterialOverridesComponent (serialized data)
        and MaterialInstance (runtime representation). It follows Unity's pattern:
        - MaterialOverridesComponent: Serializable overrides (editor/scene data)
        - MaterialInstance: Runtime material customization (created lazily)

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MATERIALOVERRIDESSYSTEM_HPP
#define MATERIALOVERRIDESSYSTEM_HPP

#include "ecs/system/system.h"
#include "ecs/internal/world.h"
#include <serialisation/guid.h>
#include <unordered_set>
#include <cstdint>

// Forward declarations
class MaterialInstanceManager;
class MaterialInstance;
class Material;
struct Texture;  // Note: Texture is a struct, not a class
class ResourceSystem;

/**
 * @brief System that synchronizes MaterialOverridesComponent with MaterialInstance
 *
 * Responsibilities:
 * 1. Lazy creation: Create MaterialInstance only when MaterialOverridesComponent has data
 * 2. Sync properties: Apply component overrides to MaterialInstance
 * 3. Load textures: Resolve texture GUIDs to loaded textures
 * 4. Cleanup: Destroy MaterialInstance when overrides are cleared or component removed
 *
 * @requirements
 * - Entity MUST have MeshRendererComponent (for base material GUID)
 * - Entity MUST have MaterialOverridesComponent (for override data)
 * - RenderSystem MUST have MaterialInstanceManager initialized
 * - ResourceSystem MUST be available for texture loading
 *
 * @usage
 * @code
 * // Add material overrides to an entity
 * auto& overrides = entity.add<MaterialOverridesComponent>();
 * overrides.floatOverrides["u_MetallicValue"] = 0.8f;
 * overrides.vec3Overrides["u_AlbedoColor"] = glm::vec3(1.0f, 0.0f, 0.0f);
 * overrides.textureOverrides["u_AlbedoMap"] = someTextureGuid;
 *
 * // MaterialOverridesSystem will:
 * // 1. Create MaterialInstance (lazy)
 * // 2. Load texture from GUID
 * // 3. Apply all overrides to MaterialInstance
 * // 4. RenderSystem will use the MaterialInstance for rendering
 * @endcode
 */
class MaterialOverridesSystem : public ecs::SystemBase
{
public:
    /**
     * @brief Get singleton instance
     */
    static MaterialOverridesSystem& Instance() {
        static MaterialOverridesSystem instance;
        return instance;
    }

    /**
     * @brief Initialize the system
     *
     * Sets up references to MaterialInstanceManager and ResourceSystem.
     */
    void Init() override;

    /**
     * @brief Update all entities with MaterialOverridesComponent
     *
     * For each entity with both MeshRendererComponent and MaterialOverridesComponent:
     * 1. Check if entity has overrides (HasOverrides())
     * 2. If has overrides && no instance: Create MaterialInstance and apply properties
     * 3. If has overrides && has instance: Update MaterialInstance properties (if changed)
     * 4. If no overrides && has instance: Destroy MaterialInstance
     *
     * @param world ECS world
     * @param dt Delta time (unused)
     */
    void Update(ecs::world& world, float dt) override;

    /**
     * @brief Cleanup system resources
     */
    void Exit() override;

private:
    /**
     * @brief Create MaterialInstance from MaterialOverridesComponent
     *
     * @param entityUID Entity unique ID (for MaterialInstanceManager lookup)
     * @param baseMaterial Base material to instance from
     * @param overrides Component containing override data
     * @return Created MaterialInstance (or nullptr on failure)
     */
    std::shared_ptr<MaterialInstance> CreateMaterialInstance(
        uint64_t entityUID,
        std::shared_ptr<Material> baseMaterial,
        const struct MaterialOverridesComponent& overrides);

    /**
     * @brief Apply MaterialOverridesComponent properties to MaterialInstance
     *
     * Syncs all override maps (floats, vec3s, vec4s, mat4s, textures) to the instance.
     *
     * @param instance Target MaterialInstance
     * @param overrides Source override data
     */
    void ApplyOverrides(
        std::shared_ptr<MaterialInstance> instance,
        const struct MaterialOverridesComponent& overrides);

    /**
     * @brief Load texture from GUID using ResourceSystem
     *
     * @param textureGuid Asset GUID
     * @return Loaded texture (or nullptr if load fails)
     */
    std::shared_ptr<Texture> LoadTexture(const Resource::Guid& textureGuid);

    /**
     * @brief Track entities that have MaterialInstances created
     *
     * This avoids per-frame MaterialInstanceManager queries.
     * Key = entity UID, Value = true if instance exists
     */
    std::unordered_set<uint64_t> m_EntitiesWithInstances;

    /// Pointer to MaterialInstanceManager (owned by RenderSystem)
    MaterialInstanceManager* m_MaterialInstanceManager = nullptr;

    /// Pointer to ResourceSystem (for texture loading)
    ResourceSystem* m_ResourceSystem = nullptr;
};

#endif // MATERIALOVERRIDESSYSTEM_HPP
