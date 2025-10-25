#ifndef PREFABDATA_HPP
#define PREFABDATA_HPP

#include "uuid/uuid.hpp"
#include "Component/Component.hpp"
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <glm/glm.hpp>

/**
 * @brief Type-erased property value for serialization
 *
 * Supports all common component property types that can be serialized
 * to YAML format for prefab storage.
 */
using SerializedPropertyValue = std::variant<
    bool,
    int,
    float,
    double,
    std::string,
    glm::vec2,
    glm::vec3,
    glm::vec4,
    glm::quat
>;

/**
 * @brief Represents a single component's serialized data in a prefab
 *
 * Stores the component type and all its property values as key-value pairs.
 * Properties are stored with their names as keys for easy lookup during
 * deserialization and property override application.
 */
struct SerializedComponent
{
    Component::ComponentType type;                          ///< Component type
    std::map<std::string, SerializedPropertyValue> properties; ///< Property name -> value

    SerializedComponent() = default;
    SerializedComponent(Component::ComponentType t) : type(t) {}

    /**
     * @brief Set a property value
     * @param name Property name (e.g., "localPosition")
     * @param value Property value
     */
    void SetProperty(const std::string& name, SerializedPropertyValue value)
    {
        properties[name] = value;
    }

    /**
     * @brief Get a property value if it exists
     * @param name Property name
     * @return Pointer to the value, or nullptr if not found
     */
    const SerializedPropertyValue* GetProperty(const std::string& name) const
    {
        auto it = properties.find(name);
        if (it != properties.end())
            return &it->second;
        return nullptr;
    }
};

/**
 * @brief Represents a single entity in a prefab hierarchy
 *
 * Stores all components attached to this entity and references to child entities.
 * This forms a tree structure representing the prefab's entity hierarchy.
 */
struct PrefabEntity
{
    std::vector<SerializedComponent> components; ///< Components attached to this entity
    std::vector<PrefabEntity> children;          ///< Child entities in the hierarchy

    /**
     * @brief Find a component by type
     * @param type Component type to find
     * @return Pointer to the component, or nullptr if not found
     */
    SerializedComponent* GetComponent(Component::ComponentType type)
    {
        for (auto& comp : components)
        {
            if (comp.type == type)
                return &comp;
        }
        return nullptr;
    }

    /**
     * @brief Find a component by type (const version)
     * @param type Component type to find
     * @return Pointer to the component, or nullptr if not found
     */
    const SerializedComponent* GetComponent(Component::ComponentType type) const
    {
        for (const auto& comp : components)
        {
            if (comp.type == type)
                return &comp;
        }
        return nullptr;
    }

    /**
     * @brief Check if this entity has a specific component type
     * @param type Component type to check
     * @return True if the component exists
     */
    bool HasComponent(Component::ComponentType type) const
    {
        return GetComponent(type) != nullptr;
    }

    /**
     * @brief Add a component to this entity
     * @param component The component to add
     */
    void AddComponent(const SerializedComponent& component)
    {
        // Remove existing component of same type if any
        for (auto it = components.begin(); it != components.end(); ++it)
        {
            if (it->type == component.type)
            {
                components.erase(it);
                break;
            }
        }
        components.push_back(component);
    }
};

/**
 * @brief Complete prefab data structure for disk storage
 *
 * This represents the entire prefab asset as stored on disk.
 * Contains metadata (UUID, name, version) and the root entity hierarchy.
 *
 * File format: YAML (.prefab extension)
 * Location: Typically in assets/prefabs/ directory
 */
struct PrefabData
{
    // Metadata
    UUID<128> uuid;           ///< Unique identifier for this prefab
    std::string name;         ///< Human-readable prefab name
    int version = 1;          ///< Prefab format version (for future compatibility)
    std::string createdDate;  ///< ISO 8601 timestamp of creation

    // Entity hierarchy
    PrefabEntity root;        ///< Root entity of the prefab hierarchy

    PrefabData() = default;

    /**
     * @brief Create a new prefab with generated UUID
     * @param prefabName Name of the prefab
     */
    explicit PrefabData(const std::string& prefabName)
        : uuid(UUID<128>::Generate())
        , name(prefabName)
    {}

    /**
     * @brief Get the UUID as a string
     * @return String representation of the UUID
     */
    std::string GetUuidString() const
    {
        return const_cast<UUID<128>&>(uuid).ToString();
    }

    /**
     * @brief Check if this prefab data is valid
     * @return True if the prefab has valid data
     */
    bool IsValid() const
    {
        return !name.empty() && version > 0;
    }
};

/**
 * @brief Helper structure for temporary prefab instance data
 *
 * Used during prefab update operations to temporarily store instance state
 * before reloading with new prefab data. This preserves entity IDs and overrides.
 */
struct PrefabInstanceSnapshot
{
    ecs::entity entityId;                                    ///< Original entity ID
    UUID<128> prefabId;                                      ///< Prefab reference
    std::vector<Component::ComponentType> addedComponents;   ///< Added components
    std::vector<Component::ComponentType> deletedComponents; ///< Deleted components

    /**
     * @brief Property override: component type, property path, and value
     */
    struct PropertyOverrideData
    {
        Component::ComponentType componentType;
        std::string propertyPath;
        SerializedPropertyValue value;
    };

    std::vector<PropertyOverrideData> propertyOverrides;     ///< Property overrides

    PrefabInstanceSnapshot() = default;

    /**
     * @brief Create a snapshot from an existing entity with PrefabComponent
     * @param entity The entity to snapshot
     * @param prefabComp The prefab component data
     */
    PrefabInstanceSnapshot(ecs::entity entity, const class PrefabComponent& prefabComp);
};

#endif // PREFABDATA_HPP
