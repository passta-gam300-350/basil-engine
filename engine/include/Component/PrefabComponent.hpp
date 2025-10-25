#ifndef PREFABCOMPONENT_HPP
#define PREFABCOMPONENT_HPP

#include "Component.hpp"
#include "uuid/uuid.hpp"
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
 * Properties are identified by component type and a path string
 * (e.g., "TransformComponent.localPosition.x").
 */
struct PropertyOverride
{
    Component::ComponentType componentType;  ///< Which component owns this property
    std::string propertyPath;                ///< Path to the property (e.g., "localPosition.x")
    PropertyValue value;                     ///< The overridden value

    PropertyOverride() = default;
    PropertyOverride(Component::ComponentType type, const std::string& path, PropertyValue val)
        : componentType(type), propertyPath(path), value(val)
    {}
};

/**
 * @brief Component that marks an entity as a prefab instance
 *
 * This component is attached to entities that are instances of a prefab.
 * It tracks:
 * - Reference to the source prefab (via UUID)
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
    UUID<128> m_PrefabGuid;

    // Components added to this instance (not in prefab)
    std::vector<Component::ComponentType> m_AddedComponents;

    // Components deleted from this instance (present in prefab)
    std::vector<Component::ComponentType> m_DeletedComponents;

    // Property overrides for this instance
    std::vector<PropertyOverride> m_OverriddenProperties;

    // Enable/disable automatic syncing when prefab changes
    bool m_AutoSync = true;

public:
    PrefabComponent() = default;

    /**
     * @brief Construct a prefab component with a specific prefab reference
     * @param prefabGuid UUID of the source prefab
     */
    explicit PrefabComponent(const UUID<128>& prefabGuid)
        : m_PrefabGuid(prefabGuid)
    {}

    ~PrefabComponent() override = default;

    ComponentType getType() const override
    {
        return ComponentType::PREFAB_INSTANCE;
    }

    /**
     * @brief Check if a component type has been added to this instance
     * @param type The component type to check
     * @return True if this component was added to the instance
     */
    bool HasAddedComponent(ComponentType type) const
    {
        for (const auto& comp : m_AddedComponents)
        {
            if (comp == type)
                return true;
        }
        return false;
    }

    /**
     * @brief Check if a component type has been deleted from this instance
     * @param type The component type to check
     * @return True if this component was deleted from the instance
     */
    bool HasDeletedComponent(ComponentType type) const
    {
        for (const auto& comp : m_DeletedComponents)
        {
            if (comp == type)
                return true;
        }
        return false;
    }

    /**
     * @brief Add a component override to this instance
     * @param type Component type being overridden
     */
    void AddComponentOverride(ComponentType type)
    {
        if (!HasAddedComponent(type))
        {
            m_AddedComponents.push_back(type);
        }
    }

    /**
     * @brief Mark a component as deleted in this instance
     * @param type Component type being deleted
     */
    void DeleteComponentOverride(ComponentType type)
    {
        if (!HasDeletedComponent(type))
        {
            m_DeletedComponents.push_back(type);
        }
    }

    /**
     * @brief Add or update a property override
     * @param componentType Type of component containing the property
     * @param propertyPath Path to the property (e.g., "localPosition.x")
     * @param value The overridden value
     */
    void SetPropertyOverride(ComponentType componentType, const std::string& propertyPath, PropertyValue value)
    {
        // Check if override already exists and update it
        for (auto& prop : m_OverriddenProperties)
        {
            if (prop.componentType == componentType && prop.propertyPath == propertyPath)
            {
                prop.value = value;
                return;
            }
        }

        // Add new override
        m_OverriddenProperties.emplace_back(componentType, propertyPath, value);
    }

    /**
     * @brief Get a property override value if it exists
     * @param componentType Type of component containing the property
     * @param propertyPath Path to the property
     * @return Pointer to the override value, or nullptr if not overridden
     */
    const PropertyValue* GetPropertyOverride(ComponentType componentType, const std::string& propertyPath) const
    {
        for (const auto& prop : m_OverriddenProperties)
        {
            if (prop.componentType == componentType && prop.propertyPath == propertyPath)
            {
                return &prop.value;
            }
        }
        return nullptr;
    }

    /**
     * @brief Remove a specific property override
     * @param componentType Type of component containing the property
     * @param propertyPath Path to the property
     * @return True if the override was found and removed
     */
    bool RemovePropertyOverride(ComponentType componentType, const std::string& propertyPath)
    {
        for (auto it = m_OverriddenProperties.begin(); it != m_OverriddenProperties.end(); ++it)
        {
            if (it->componentType == componentType && it->propertyPath == propertyPath)
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
     * @return String representation of the prefab UUID
     */
    std::string GetPrefabGuidString() const
    {
        return const_cast<UUID<128>&>(m_PrefabGuid).ToString();
    }
};

#endif // PREFABCOMPONENT_HPP
