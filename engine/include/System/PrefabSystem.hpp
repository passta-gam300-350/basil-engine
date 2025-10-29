#ifndef PREFABSYSTEM_HPP
#define PREFABSYSTEM_HPP

#include "ecs/system/system.h"
#include "ecs/internal/world.h"
#include "Prefab/PrefabData.hpp"
#include "Component/PrefabComponent.hpp"
#include "serialisation/guid.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>

/**
 * @brief System responsible for prefab instantiation, syncing, and lifecycle management
 *
 * The PrefabSystem manages the relationship between prefab assets (templates) and
 * prefab instances (entities in the scene). It handles:
 *
 * 1. **Instantiation**: Creating entities from prefab templates
 * 2. **Syncing**: Updating instances when prefab assets change
 * 3. **Override Tracking**: Detecting and preserving instance-specific modifications
 * 4. **Serialization**: Loading and saving prefab data from/to disk
 *
 * # Usage Example
 * @code
 * // Instantiate a prefab
 * UUID<128> prefabId = LoadPrefabId("assets/prefabs/enemy.prefab");
 * ecs::entity instance = PrefabSystem::InstantiatePrefab(world, prefabId, glm::vec3(0, 0, 0));
 *
 * // Sync all instances of a prefab after editing
 * PrefabSystem::SyncPrefab(world, prefabId);
 *
 * // Revert instance overrides
 * PrefabSystem::RevertAllOverrides(world, instance);
 * @endcode
 *
 * # Integration with Scene Graph
 * The prefab system uses the existing SceneGraph and RelationshipComponent
 * to manage entity hierarchies. Prefab instances maintain parent-child
 * relationships through the standard scene graph API.
 *
 * @see PrefabComponent
 * @see PrefabData
 * @see SceneGraph
 */
class PrefabSystem : public ecs::SystemBase
{
public:
    PrefabSystem() = default;
    ~PrefabSystem() override = default;

    // System lifecycle
    void Init() override;
    void Update(ecs::world& world, float deltaTime) override;
    void Exit() override;

    // ========================
    // Core Prefab Operations
    // ========================

    /**
     * @brief Instantiate a prefab at a specific position
     *
     * Creates a new entity hierarchy from a prefab template. The root entity
     * will be given a PrefabComponent to track its connection to the prefab.
     *
     * @param world The ECS world to create entities in
     * @param prefabId Guid of the prefab to instantiate
     * @param position World position for the root entity
     * @return The root entity of the created instance (invalid if failed)
     */
    static ecs::entity InstantiatePrefab(ecs::world& world, const Resource::Guid& prefabId, const glm::vec3& position = glm::vec3(0));

    /**
     * @brief Instantiate a prefab with a specific entity ID (for loading from disk)
     *
     * This variant allows specifying the entity ID, which is necessary when
     * loading a scene from disk where entity IDs must be preserved.
     *
     * @param world The ECS world to create entities in
     * @param prefabId Guid of the prefab to instantiate
     * @param rootEntityId Desired entity ID for the root entity
     * @param position World position for the root entity
     * @return The root entity of the created instance (invalid if failed)
     */
    static ecs::entity InstantiatePrefabWithId(ecs::world& world, const Resource::Guid& prefabId, ecs::entity rootEntityId, const glm::vec3& position = glm::vec3(0));

    /**
     * @brief Sync all instances of a specific prefab
     *
     * Called when a prefab asset is modified. Updates all instances to match
     * the new prefab data while preserving property overrides.
     *
     * Algorithm:
     * 1. Find all entities with PrefabComponent matching prefabId
     * 2. For each instance:
     *    - Snapshot current overrides
     *    - Reload prefab data
     *    - Apply base prefab data
     *    - Reapply overrides
     *
     * @param world The ECS world containing instances
     * @param prefabId Guid of the prefab that changed
     * @return Number of instances successfully synced
     */
    static int SyncPrefab(ecs::world& world, const Resource::Guid& prefabId);

    /**
     * @brief Sync a single prefab instance
     *
     * Reloads prefab data and applies it to the instance while preserving overrides.
     *
     * @param world The ECS world
     * @param instance The prefab instance entity
     * @return True if sync was successful
     */
    static bool SyncInstance(ecs::world& world, ecs::entity instance);

    // ========================
    // Override Management
    // ========================

    /**
     * @brief Mark a property as overridden in an instance
     *
     * Records that this instance has a custom value for this property,
     * preventing it from being overwritten during prefab sync.
     *
     * @param world The ECS world
     * @param instance The prefab instance entity
     * @param componentType Type of component containing the property
     * @param propertyPath Path to the property (e.g., "localPosition.x")
     * @param value The overridden value
     */
    static void MarkPropertyOverridden(ecs::world& world, ecs::entity instance,
                                       std::uint32_t componentTypeHash,
                                       const std::string& componentTypeName,
                                       const std::string& propertyPath,
                                       PropertyValue value);

    /**
     * @brief Revert a specific property override
     *
     * Removes the override and restores the property value from the prefab.
     *
     * @param world The ECS world
     * @param instance The prefab instance entity
     * @param componentType Type of component containing the property
     * @param propertyPath Path to the property
     * @return True if the override was found and reverted
     */
    static bool RevertOverride(ecs::world& world, ecs::entity instance,
                               std::uint32_t componentTypeHash,
                               const std::string& propertyPath);

    /**
     * @brief Revert all property overrides for an instance
     *
     * Removes all overrides and restores all properties to prefab values.
     *
     * @param world The ECS world
     * @param instance The prefab instance entity
     */
    static void RevertAllOverrides(ecs::world& world, ecs::entity instance);

    /**
     * @brief Apply an instance's modifications to its prefab
     *
     * Updates the prefab asset with changes from this instance.
     * This is used for the "Apply to Prefab" workflow.
     *
     * @param world The ECS world
     * @param instance The prefab instance entity
     * @param prefabPath Path to save the updated prefab
     * @return True if the prefab was successfully updated
     */
    static bool ApplyInstanceToPrefab(ecs::world& world, ecs::entity instance, const std::string& prefabPath);

    // ========================
    // Queries
    // ========================

    /**
     * @brief Get all instances of a specific prefab
     *
     * @param world The ECS world
     * @param prefabId Guid of the prefab
     * @return Vector of entity handles
     */
    static std::vector<ecs::entity> GetAllInstances(ecs::world& world, const Resource::Guid& prefabId);

    /**
     * @brief Check if an entity is an instance of a specific prefab
     *
     * @param world The ECS world
     * @param entity The entity to check
     * @param prefabId Guid of the prefab
     * @return True if the entity is an instance of the prefab
     */
    static bool IsInstanceOf(ecs::world& world, ecs::entity entity, const Resource::Guid& prefabId);

    /**
     * @brief Check if an entity is any prefab instance
     *
     * @param world The ECS world
     * @param entity The entity to check
     * @return True if the entity has a PrefabComponent
     */
    static bool IsPrefabInstance(ecs::world& world, ecs::entity entity);

    // ========================
    // Prefab Data Management
    // ========================

    /**
     * @brief Load a prefab from disk
     *
     * @param prefabPath Path to the .prefab file
     * @return PrefabData structure (check IsValid())
     */
    static PrefabData LoadPrefabFromFile(const std::string& prefabPath);

    /**
     * @brief Save a prefab to disk
     *
     * @param data The prefab data to save
     * @param prefabPath Path to save the .prefab file
     * @return True if save was successful
     */
    static bool SavePrefabToFile(const PrefabData& data, const std::string& prefabPath);

    /**
     * @brief Create a prefab from an existing entity hierarchy
     *
     * Serializes an entity and its children into a prefab asset.
     *
     * @param world The ECS world
     * @param rootEntity Root entity to convert to prefab
     * @param prefabName Name for the new prefab
     * @param prefabPath Path to save the prefab file
     * @return Guid of the created prefab (invalid if failed)
     */
    static Resource::Guid CreatePrefabFromEntity(ecs::world& world, ecs::entity rootEntity,
                                                 const std::string& prefabName,
                                                 const std::string& prefabPath);

private:
    // Internal helper methods

    /**
     * @brief Recursively create entities from prefab data
     */
    static ecs::entity InstantiateEntity(ecs::world& world, const PrefabEntity& prefabEntity,
                                         ecs::entity parent, const Resource::Guid& prefabId);

    /**
     * @brief Apply prefab data to an existing entity (for syncing)
     */
    static void ApplyPrefabDataToEntity(ecs::world& world, ecs::entity entity,
                                        const PrefabEntity& prefabEntity,
                                        const PrefabComponent* prefabComp);

    /**
     * @brief Serialize an entity hierarchy to PrefabData
     */
    static PrefabEntity SerializeEntityHierarchy(ecs::world& world, ecs::entity entity);

    /**
     * @brief Apply component data to an entity
     */
    static void ApplyComponentData(ecs::world& world, ecs::entity entity,
                                    const SerializedComponent& componentData);

    /**
     * @brief Check if a property is overridden
     */
    static bool IsPropertyOverridden(const PrefabComponent* prefabComp,
                                     std::uint32_t componentTypeHash,
                                     const std::string& propertyPath);

    // Prefab cache (UUID -> PrefabData)
    static std::unordered_map<std::string, PrefabData> s_PrefabCache;
};

#endif // PREFABSYSTEM_HPP
