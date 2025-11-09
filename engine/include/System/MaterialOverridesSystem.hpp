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
#include <native/native.h>
#include <unordered_set>
#include <cstdint>

// Forward declarations
class MaterialPropertyBlock;

/**
 * @brief System that synchronizes MaterialOverridesComponent with MaterialPropertyBlock
 *
 * Responsibilities:
 * 1. Lazy creation: Create MaterialPropertyBlock only when MaterialOverridesComponent has data
 * 2. Sync properties: Apply component overrides to MaterialPropertyBlock
 * 3. Cleanup: Clear MaterialPropertyBlock when overrides are cleared or component removed
 *
 * @requirements
 * - Entity MUST have MeshRendererComponent (for base material GUID)
 * - Entity MUST have MaterialOverridesComponent (for override data)
 * - RenderSystem MUST provide MaterialPropertyBlock management
 *
 * @note MaterialPropertyBlock is used instead of MaterialInstance to preserve GPU instancing.
 *       Property blocks are lightweight and applied per-draw call by SceneRenderer.
 *
 * @usage
 * @code
 * // Add material overrides to an entity
 * auto& overrides = entity.add<MaterialOverridesComponent>();
 * overrides.floatOverrides["u_MetallicValue"] = 0.8f;
 * overrides.vec3Overrides["u_AlbedoColor"] = glm::vec3(1.0f, 0.0f, 0.0f);
 *
 * // MaterialOverridesSystem will:
 * // 1. Create MaterialPropertyBlock (lazy)
 * // 2. Apply all overrides to MaterialPropertyBlock
 * // 3. SceneRenderer will apply property block after base material during rendering
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
    void Update(ecs::world& world, [[maybe_unused]] float dt) override;

    /**
     * @brief Cleanup system resources
     */
    void Exit() override;

private:
    /**
     * @brief Apply MaterialOverridesComponent properties to MaterialPropertyBlock
     *
     * Syncs all override maps (floats, vec3s, vec4s, mat4s) to the property block.
     *
     * @param propBlock Target MaterialPropertyBlock
     * @param overrides Source override data
     */
    void ApplyOverridesToPropertyBlock(
        std::shared_ptr<MaterialPropertyBlock> propBlock,
        const struct MaterialOverridesComponent& overrides);

    /**
     * @brief Track entities that have MaterialPropertyBlocks created
     *
     * This avoids per-frame property block queries.
     * Key = entity UID
     */
    std::unordered_set<uint64_t> m_EntitiesWithInstances;
};

#endif // MATERIALOVERRIDESSYSTEM_HPP
