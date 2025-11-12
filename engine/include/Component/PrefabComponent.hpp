#ifndef PREFABCOMPONENT_HPP
#define PREFABCOMPONENT_HPP

#include "Component.hpp"
#include "rsc-core/guid.hpp"
#include <vector>
#include <string>
#include <variant>
#include <glm/glm.hpp>

/**
 * @brief Type-erased property value container for prefab overrides
 *
 * Stores various types of component property values that can be overridden
 * in prefab instances. Supports common types used in game engine components.
 */
using PropertyValue = std::variant<
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
 * @brief Represents an overridden property in a prefab instance
 *
 * Tracks a single property that differs from the prefab template.
 * Properties are identified by reflection type hash and a path string
 * (e.g., "m_Translation" or "m_Translation.x").
 *
 * NOTE: Now uses reflection type hash (entt::id_type) instead of Component::ComponentType
 * since actual gameplay components use the reflection system.
 */
struct PropertyOverride
{
    std::uint32_t componentTypeHash = 0;     ///< Reflection type hash of component
    std::string componentTypeName;            ///< Human-readable component type name
    std::string propertyPath;                ///< Path to the property (e.g., "m_Translation")
    PropertyValue value;                     ///< The overridden value

    PropertyOverride() = default;
    PropertyOverride(std::uint32_t typeHash, std::string typeName, const std::string& path, PropertyValue val)
        : componentTypeHash(typeHash), componentTypeName(std::move(typeName)), propertyPath(path), value(val)
    {}
};

/**
 * @brief Component that marks an entity as a prefab instance
 *
 * This component is attached to entities that are instances of a prefab.
 * It tracks:
 * - Reference to the source prefab (via rp::BasicIndexedGuid)
 * - Components added to this instance (not in original prefab)
 * - Components deleted from this instance (present in prefab)
 * - Property overrides (values different from prefab)
 *
 * This component is invisible in the editor hierarchy but is used
 * by the PrefabSystem to synchronize instances when prefabs change.
 *
 * @see PrefabSystem
 */
class PrefabComponent : public Component
{
public:
    // Reference to the prefab resource
    rp::BasicIndexedGuid m_PrefabGuid;

    // Components added to this instance (not in prefab) - stored as reflection type hashes
    std::vector<std::uint32_t> m_AddedComponents;

    // Components deleted from this instance (present in prefab) - stored as reflection type hashes
    std::vector<std::uint32_t> m_DeletedComponents;

    // Property overrides for this instance
    std::vector<PropertyOverride> m_OverriddenProperties;

    // Enable/disable automatic syncing when prefab changes
    bool m_AutoSync = true;

public:
    PrefabComponent() = default;

    /**
     * @brief Construct a prefab component with a specific prefab reference
     * @param prefabGuid BasicIndexedGuid of the source prefab
     */
    explicit PrefabComponent(const rp::BasicIndexedGuid& prefabGuid)
        : m_PrefabGuid(prefabGuid)
    {}

    ~PrefabComponent() override = default;

    ComponentType getType() const override
    {
        return ComponentType::PREFAB_INSTANCE;
    }

    /**
     * @brief Check if a component type has been added to this instance
     * @param typeHash Reflection type hash to check
     * @return True if this component was added to the instance
     */
    bool HasAddedComponent(std::uint32_t typeHash) const
    {
        for (const auto& comp : m_AddedComponents)
        {
            if (comp == typeHash)
                return true;
        }
        return false;
    }

    /**
     * @brief Check if a component type has been deleted from this instance
     * @param typeHash Reflection type hash to check
     * @return True if this component was deleted from the instance
     */
    bool HasDeletedComponent(std::uint32_t typeHash) const
    {
        for (const auto& comp : m_DeletedComponents)
        {
            if (comp == typeHash)
                return true;
        }
        return false;
    }

    /**
     * @brief Add a component override to this instance
     * @param typeHash Reflection type hash of component being added
     */
    void AddComponentOverride(std::uint32_t typeHash)
    {
        if (!HasAddedComponent(typeHash))
        {
            m_AddedComponents.push_back(typeHash);
        }
    }

    /**
     * @brief Mark a component as deleted in this instance
     * @param typeHash Reflection type hash of component being deleted
     */
    void DeleteComponentOverride(std::uint32_t typeHash)
    {
        if (!HasDeletedComponent(typeHash))
        {
            m_DeletedComponents.push_back(typeHash);
        }
    }

    /**
     * @brief Add or update a property override
     * @param typeHash Reflection type hash of component containing the property
     * @param typeName Human-readable component type name
     * @param propertyPath Path to the property (e.g., "m_Translation")
     * @param value The overridden value
     */
    void SetPropertyOverride(std::uint32_t typeHash, const std::string& typeName, const std::string& propertyPath, PropertyValue value)
    {
        // Check if override already exists and update it
        for (auto& prop : m_OverriddenProperties)
        {
            if (prop.componentTypeHash == typeHash && prop.propertyPath == propertyPath)
            {
                prop.value = value;
                return;
            }
        }

        // Add new override
        m_OverriddenProperties.emplace_back(typeHash, typeName, propertyPath, value);
    }

    /**
     * @brief Get a property override value if it exists
     * @param typeHash Reflection type hash of component containing the property
     * @param propertyPath Path to the property
     * @return Pointer to the override value, or nullptr if not overridden
     */
    const PropertyValue* GetPropertyOverride(std::uint32_t typeHash, const std::string& propertyPath) const
    {
        for (const auto& prop : m_OverriddenProperties)
        {
            if (prop.componentTypeHash == typeHash && prop.propertyPath == propertyPath)
            {
                return &prop.value;
            }
        }
        return nullptr;
    }

    /**
     * @brief Remove a specific property override
     * @param typeHash Reflection type hash of component containing the property
     * @param propertyPath Path to the property
     * @return True if the override was found and removed
     */
    bool RemovePropertyOverride(std::uint32_t typeHash, const std::string& propertyPath)
    {
        for (auto it = m_OverriddenProperties.begin(); it != m_OverriddenProperties.end(); ++it)
        {
            if (it->componentTypeHash == typeHash && it->propertyPath == propertyPath)
            {
                m_OverriddenProperties.erase(it);
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Clear all property overrides
     */
    void ClearAllPropertyOverrides()
    {
        m_OverriddenProperties.clear();
    }

    /**
     * @brief Check if this component should be visible in the editor
     * @return False - prefab components are hidden from normal editor view
     */
    bool inEditor() override
    {
        return false; // Hidden in editor by default
    }

    /**
     * @brief Get the prefab GUID as a string
     * @return String representation of the prefab GUID
     */
    std::string GetPrefabGuidString() const
    {
        return m_PrefabGuid.m_guid.to_hex();
    }
};

#endif // PREFABCOMPONENT_HPP
