#ifndef COMPONENTREGISTRY_HPP
#define COMPONENTREGISTRY_HPP

#include "Ecs/internal/reflection.h"
#include "Prefab/PrefabData.hpp"
#include "Ecs/ecs.h"
#include <functional>
#include <unordered_map>
#include <optional>

/**
 * @brief Registry for component serialization/deserialization in the prefab system
 *
 * Provides mapping between component type hashes and serialization functions.
 * Uses the existing ReflectionRegistry as the underlying storage but adds
 * prefab-specific functionality for component data extraction and application.
 *
 * This is a thin wrapper around ReflectionRegistry that provides type-safe
 * access to component serialization for the prefab system.
 */
class ComponentRegistry
{
public:
    using TypeID = std::uint32_t; // entt::id_type

    /**
     * @brief Serialize a component from an entity
     * @param entity The entity containing the component
     * @param typeHash The reflection type hash of the component
     * @return Serialized component data, or nullopt if component doesn't exist
     */
    static std::optional<SerializedComponent> SerializeComponent(
        ecs::entity entity,
        TypeID typeHash);

    /**
     * @brief Get the component type name from a type hash
     * @param typeHash The reflection type hash
     * @return Component type name (e.g., "TransformComponent"), or empty string if not found
     */
    static std::string GetTypeName(TypeID typeHash)
    {
        auto& names = ReflectionRegistry::TypeNames();
        auto it = names.find(typeHash);
        return (it != names.end()) ? it->second : std::string{};
    }

    /**
     * @brief Get the type hash from a type name
     * @param typeName Component type name (e.g., "TransformComponent")
     * @return Type hash, or 0 if not found
     */
    static TypeID GetTypeHash(const std::string& typeName)
    {
        auto& names = ReflectionRegistry::TypeNames();
        for (const auto& [hash, name] : names)
        {
            if (name == typeName)
                return hash;
        }
        return 0;
    }

    /**
     * @brief Check if a component type is registered
     * @param typeHash The reflection type hash
     * @return True if the component type is registered in the reflection system
     */
    static bool IsTypeRegistered(TypeID typeHash)
    {
        auto& types = ReflectionRegistry::types();
        return types.find(typeHash) != types.end();
    }

    /**
     * @brief Get type hash for a given component type at compile time
     * @tparam T Component type
     * @return Type hash for T
     */
    template<typename T>
    static constexpr TypeID GetTypeID()
    {
        return ReflectionRegistry::GetTypeID<T>();
    }

    /**
     * @brief Serialize a specific component from an entity (type-safe version)
     * @tparam T Component type
     * @param entity The entity containing the component
     * @return Serialized component data, or nullopt if component doesn't exist
     */
    template<typename T>
    static std::optional<SerializedComponent> SerializeComponent(ecs::entity entity)
    {
        return SerializeComponent(entity, GetTypeID<T>());
    }

    /**
     * @brief Extract property value from a serialized component
     * @param component The serialized component
     * @param propertyPath Path to the property (e.g., "m_Position.x")
     * @return Property value, or nullopt if not found
     */
    static std::optional<SerializedPropertyValue> GetProperty(
        const SerializedComponent& component,
        const std::string& propertyPath);

    /**
     * @brief Set property value in a serialized component
     * @param component The serialized component to modify
     * @param propertyPath Path to the property (e.g., "m_Position.x")
     * @param value The value to set
     * @return True if successful, false if property path is invalid
     */
    static bool SetProperty(
        SerializedComponent& component,
        const std::string& propertyPath,
        SerializedPropertyValue value);

    /**
     * @brief Check if an entity has a specific component type
     * @param entity The entity to check
     * @param typeHash The reflection type hash
     * @return True if the entity has the component
     */
    static bool EntityHasComponent(ecs::entity entity, TypeID typeHash);

    /**
     * @brief Remove a component from an entity by type hash
     * @param entity The entity to modify
     * @param typeHash The reflection type hash of the component to remove
     * @return True if the component was removed, false if it didn't exist
     */
    static bool RemoveComponent(ecs::entity entity, TypeID typeHash);

    /**
     * @brief Get all component type hashes on an entity
     * @param entity The entity to query
     * @return Vector of type hashes for all components on the entity
     */
    static std::vector<TypeID> GetAllComponentTypes(ecs::entity entity);

private:
    /**
     * @brief Helper to convert entt::meta_any to SerializedPropertyValue
     * @param metaValue The meta value from reflection
     * @return Serialized value, or nullopt if type not supported
     */
    static std::optional<SerializedPropertyValue> MetaAnyToPropertyValue(
        const entt::meta_any& metaValue);

    /**
     * @brief Helper to convert SerializedPropertyValue to entt::meta_any
     * @param value The serialized value
     * @param targetType The target meta type
     * @return Meta any value, or empty if conversion fails
     */
    static entt::meta_any PropertyValueToMetaAny(
        const SerializedPropertyValue& value,
        entt::meta_type targetType);
};

#endif // COMPONENTREGISTRY_HPP
