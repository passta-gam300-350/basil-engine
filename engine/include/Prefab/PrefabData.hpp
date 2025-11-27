#ifndef PREFABDATA_HPP
#define PREFABDATA_HPP


#include "Component/Component.hpp"
#include <rsc-core/rp.hpp>
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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
    glm::quat,
    rp::BasicIndexedGuid
>;

/**
 * @brief Represents a single component's serialized data in a prefab
 *
 * Uses reflection-based type identification (entt::id_type hash + human-readable name).
 * Properties are stored with their names as keys for easy lookup during
 * deserialization and property override application.
 *
 * NOTE: Replaced Component::ComponentType enum with reflection types since actual
 * gameplay components (TransformComponent, MeshRendererComponent, etc.) are
 * registered via reflection, not the legacy ComponentType enum.
 */
struct SerializedComponent
{
    std::uint32_t typeHash = 0;                             ///< Reflection type hash (entt::id_type)
    std::string typeName;                                   ///< Human-readable type name (e.g., "TransformComponent")
    std::map<std::string, SerializedPropertyValue> properties; ///< Property name -> value

    SerializedComponent() = default;
    SerializedComponent(std::uint32_t hash, std::string name)
        : typeHash(hash), typeName(std::move(name)) {}

    /**
     * @brief Set a property value
     * @param name Property name (e.g., "m_Translation")
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
 * @brief Represents a reference to a nested prefab instance within a parent prefab
 *
 * Unity-style nested prefabs: Instead of expanding the nested prefab's entities,
 * we store a reference to the prefab asset and track any overrides applied at this level.
 * When instantiating, we recursively instantiate nested prefabs and apply overrides.
 *
 * Example hierarchy:
 *   Tank (prefab)
 *     └─ Turret (nested prefab reference)
 *          └─ Barrel (in Turret's prefab, not expanded here)
 */
struct NestedPrefabReference
{
    rp::BasicIndexedGuid prefabGuid;             ///< GUID of the nested prefab asset
    std::string prefabName;                      ///< Human-readable name for debugging

    // Transform override for positioning the nested prefab within parent
    // (applied on top of the nested prefab's root transform)
    glm::vec3 positionOverride = glm::vec3(0.0f);
    glm::quat rotationOverride = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // identity quaternion
    glm::vec3 scaleOverride = glm::vec3(1.0f);

    // Property overrides specific to this nested prefab instance
    // (applied to the nested prefab's entities during instantiation)
    std::vector<SerializedComponent> componentOverrides;

    NestedPrefabReference() = default;
    NestedPrefabReference(const rp::BasicIndexedGuid& guid, std::string name)
        : prefabGuid(guid), prefabName(std::move(name)) {}
};

/**
 * @brief Represents a single entity in a prefab hierarchy
 *
 * Stores all components attached to this entity and references to child entities.
 * This forms a tree structure representing the prefab's entity hierarchy.
 *
 * **Nested Prefab Support (Unity 2018.3+ style):**
 * - Regular child entities are stored in `children`
 * - Nested prefab instances are stored in `nestedPrefabs`
 * - Nested prefabs are NOT expanded - they remain as references
 * - At instantiation time, nested prefabs are recursively instantiated
 */
struct PrefabEntity
{
    std::vector<SerializedComponent> components; ///< Components attached to this entity
    std::vector<PrefabEntity> children;          ///< Regular child entities (fully expanded)
    std::vector<NestedPrefabReference> nestedPrefabs; ///< Nested prefab references (NOT expanded)

    /**
     * @brief Find a component by type hash
     * @param typeHash Reflection type hash to find
     * @return Pointer to the component, or nullptr if not found
     */
    SerializedComponent* GetComponent(std::uint32_t typeHash)
    {
        for (auto& comp : components)
        {
            if (comp.typeHash == typeHash)
                return &comp;
        }
        return nullptr;
    }

    /**
     * @brief Find a component by type hash (const version)
     * @param typeHash Reflection type hash to find
     * @return Pointer to the component, or nullptr if not found
     */
    const SerializedComponent* GetComponent(std::uint32_t typeHash) const
    {
        for (const auto& comp : components)
        {
            if (comp.typeHash == typeHash)
                return &comp;
        }
        return nullptr;
    }

    /**
     * @brief Find a component by type name
     * @param typeName Component type name (e.g., "TransformComponent")
     * @return Pointer to the component, or nullptr if not found
     */
    SerializedComponent* GetComponentByName(const std::string& typeName)
    {
        for (auto& comp : components)
        {
            if (comp.typeName == typeName)
                return &comp;
        }
        return nullptr;
    }

    /**
     * @brief Find a component by type name (const version)
     * @param typeName Component type name
     * @return Pointer to the component, or nullptr if not found
     */
    const SerializedComponent* GetComponentByName(const std::string& typeName) const
    {
        for (const auto& comp : components)
        {
            if (comp.typeName == typeName)
                return &comp;
        }
        return nullptr;
    }

    /**
     * @brief Check if this entity has a specific component type
     * @param typeHash Reflection type hash to check
     * @return True if the component exists
     */
    bool HasComponent(std::uint32_t typeHash) const
    {
        return GetComponent(typeHash) != nullptr;
    }

    /**
     * @brief Check if this entity has a specific component type by name
     * @param typeName Component type name to check
     * @return True if the component exists
     */
    bool HasComponentByName(const std::string& typeName) const
    {
        return GetComponentByName(typeName) != nullptr;
    }

    /**
     * @brief Add a component to this entity
     * @param component The component to add (replaces existing if same type)
     */
    void AddComponent(const SerializedComponent& component)
    {
        // Remove existing component of same type hash if any
        for (auto it = components.begin(); it != components.end(); ++it)
        {
            if (it->typeHash == component.typeHash)
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
 * Contains metadata (GUID, name, version) and the root entity hierarchy.
 *
 * File format: YAML (.prefab extension)
 * Location: Typically in assets/prefabs/ directory
 */
struct PrefabData
{
    // Metadata
    rp::BasicIndexedGuid guid;  ///< Unique identifier for this prefab
    std::string name;           ///< Human-readable prefab name
    int version = 1;            ///< Prefab format version (for future compatibility)
    std::string createdDate;    ///< ISO 8601 timestamp of creation

    // Entity hierarchy
    PrefabEntity root;          ///< Root entity of the prefab hierarchy

    PrefabData() = default;

    /**
     * @brief Create a new prefab with generated GUID
     * @param prefabName Name of the prefab
     * @param typeIndex Type index for the prefab resource type
     */
    explicit PrefabData(const std::string& prefabName, std::uint64_t typeIndex = 0)
        : guid{rp::Guid::generate(), typeIndex}
        , name(prefabName)
    {}

    /**
     * @brief Get the GUID as a string
     * @return String representation of the GUID
     */
    std::string GetGuidString() const
    {
        return guid.m_guid.to_hex();
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
    rp::BasicIndexedGuid prefabId;                                      ///< Prefab reference
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
};

#endif // PREFABDATA_HPP
