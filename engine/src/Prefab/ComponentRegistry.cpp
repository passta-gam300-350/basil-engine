#include "Prefab/ComponentRegistry.hpp"
#include "Ecs/ecs.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

std::optional<SerializedComponent> ComponentRegistry::SerializeComponent(
    ecs::entity entity,
    TypeID typeHash)
{
    // Check if type is registered
    auto& types = ReflectionRegistry::types();
    auto typeIt = types.find(typeHash);
    if (typeIt == types.end())
    {
        return std::nullopt; // Type not registered
    }

    entt::meta_type metaType = typeIt->second;
    std::string typeName = GetTypeName(typeHash);

    // Get the raw component data
    // Use the entity's get_reflectible_components() method to get component data
    auto components = entity.get_reflectible_components();

    // Find the component with matching type hash
    std::byte* componentData = nullptr;
    for (auto& [hash, data] : components)
    {
        if (hash == typeHash)
        {
            componentData = data.get();
            break;
        }
    }

    if (!componentData)
    {
        return std::nullopt; // Entity doesn't have this component
    }

    // Create serialized component
    SerializedComponent serialized(typeHash, typeName);

    // Get meta any from component data
    entt::meta_any metaAny = metaType.from_void(componentData);

    // Iterate over all data members and serialize them
    auto fieldNames = ReflectionRegistry::GetFieldNames(typeHash);
    for (auto [fieldId, fieldInfo] : metaType.data())
    {
        auto fieldIt = fieldNames.find(fieldId);
        if (fieldIt == fieldNames.end())
            continue; // Field not registered

        std::string fieldName = fieldIt->second;
        entt::meta_any fieldValue = fieldInfo.get(metaAny);

        // Convert to SerializedPropertyValue
        auto propValue = MetaAnyToPropertyValue(fieldValue);
        if (propValue)
        {
            serialized.SetProperty(fieldName, *propValue);
        }
    }

    return serialized;
}

std::optional<SerializedPropertyValue> ComponentRegistry::GetProperty(
    const SerializedComponent& component,
    const std::string& propertyPath)
{
    // Check if property path has nested access (e.g., "m_Translation.x")
    size_t dotPos = propertyPath.find('.');
    if (dotPos != std::string::npos)
    {
        // Split into parent and child
        std::string parentPath = propertyPath.substr(0, dotPos);
        std::string childField = propertyPath.substr(dotPos + 1);

        // Get parent property
        const auto* parentValue = component.GetProperty(parentPath);
        if (!parentValue)
            return std::nullopt;

        // Handle nested access for common types
        return std::visit([&childField](auto&& value) -> std::optional<SerializedPropertyValue> {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, glm::vec3>)
            {
                if (childField == "x") return value.x;
                if (childField == "y") return value.y;
                if (childField == "z") return value.z;
            }
            else if constexpr (std::is_same_v<T, glm::vec4>)
            {
                if (childField == "x") return value.x;
                if (childField == "y") return value.y;
                if (childField == "z") return value.z;
                if (childField == "w") return value.w;
            }
            else if constexpr (std::is_same_v<T, glm::vec2>)
            {
                if (childField == "x") return value.x;
                if (childField == "y") return value.y;
            }

            return std::nullopt;  // Unsupported type or field
        }, *parentValue);
    }

    // Direct property access (no nesting)
    const auto* value = component.GetProperty(propertyPath);
    if (value)
        return *value;

    return std::nullopt;
}

bool ComponentRegistry::SetProperty(
    SerializedComponent& component,
    const std::string& propertyPath,
    SerializedPropertyValue value)
{
    // Check if property path has nested access (e.g., "m_Translation.x")
    size_t dotPos = propertyPath.find('.');
    if (dotPos != std::string::npos)
    {
        // Split into parent and child
        std::string parentPath = propertyPath.substr(0, dotPos);
        std::string childField = propertyPath.substr(dotPos + 1);

        // Get parent property
        auto* parentValue = component.GetProperty(parentPath);
        if (!parentValue)
            return false;

        // Modify nested field
        bool modified = std::visit([&childField, &value](auto&& parentVal) -> bool {
            using T = std::decay_t<decltype(parentVal)>;

            if constexpr (std::is_same_v<T, glm::vec3>)
            {
                if (auto* floatVal = std::get_if<float>(&value))
                {
                    if (childField == "x") { parentVal.x = *floatVal; return true; }
                    if (childField == "y") { parentVal.y = *floatVal; return true; }
                    if (childField == "z") { parentVal.z = *floatVal; return true; }
                }
            }
            else if constexpr (std::is_same_v<T, glm::vec4>)
            {
                if (auto* floatVal = std::get_if<float>(&value))
                {
                    if (childField == "x") { parentVal.x = *floatVal; return true; }
                    if (childField == "y") { parentVal.y = *floatVal; return true; }
                    if (childField == "z") { parentVal.z = *floatVal; return true; }
                    if (childField == "w") { parentVal.w = *floatVal; return true; }
                }
            }
            else if constexpr (std::is_same_v<T, glm::vec2>)
            {
                if (auto* floatVal = std::get_if<float>(&value))
                {
                    if (childField == "x") { parentVal.x = *floatVal; return true; }
                    if (childField == "y") { parentVal.y = *floatVal; return true; }
                }
            }

            return false;
        }, *parentValue);

        if (modified)
        {
            // Update parent property with modified value
            component.SetProperty(parentPath, *parentValue);
            return true;
        }
        return false;
    }

    // Direct property access (no nesting)
    component.SetProperty(propertyPath, value);
    return true;
}

bool ComponentRegistry::EntityHasComponent(ecs::entity entity, TypeID typeHash)
{
    auto components = entity.get_reflectible_components();
    for (const auto& [hash, data] : components)
    {
        if (hash == typeHash)
            return true;
    }
    return false;
}

bool ComponentRegistry::RemoveComponent(ecs::entity entity, TypeID typeHash)
{
    // Use reflection registry to find the meta type
    auto typeIt = ReflectionRegistry::types().find(typeHash);
    if (typeIt == ReflectionRegistry::types().end())
        return false;  // Unknown component type

    auto metaType = typeIt->second;

    // Get the ECS registry
    auto& world = Engine::GetWorld();
    auto& registry = world.impl.get_registry();
    entt::entity enttEntity = ecs::world::detail::entt_entity_cast(entity);

    // Check if entity has the component
    auto* storage = registry.storage(typeHash);
    if (!storage || !storage->contains(enttEntity))
        return false;  // Entity doesn't have this component

    // Remove the component using EnTT's remove function
    // EnTT storage has a remove() method
    storage->remove(enttEntity);

    return true;
}

std::vector<ComponentRegistry::TypeID> ComponentRegistry::GetAllComponentTypes(ecs::entity entity)
{
    std::vector<TypeID> result;
    auto components = entity.get_reflectible_components();

    for (const auto& [hash, data] : components)
    {
        result.push_back(hash);
    }

    return result;
}

std::optional<SerializedPropertyValue> ComponentRegistry::MetaAnyToPropertyValue(
    const entt::meta_any& metaValue)
{
    auto metaType = metaValue.type();

    // Try to cast to supported types
    // bool
    if (auto ptr = metaValue.try_cast<bool>())
        return *ptr;

    // int
    if (auto ptr = metaValue.try_cast<int>())
        return *ptr;

    // float
    if (auto ptr = metaValue.try_cast<float>())
        return *ptr;

    // double
    if (auto ptr = metaValue.try_cast<double>())
        return *ptr;

    // std::string
    if (auto ptr = metaValue.try_cast<std::string>())
        return *ptr;

    // glm::vec2
    if (auto ptr = metaValue.try_cast<glm::vec2>())
        return *ptr;

    // glm::vec3
    if (auto ptr = metaValue.try_cast<glm::vec3>())
        return *ptr;

    // glm::vec4
    if (auto ptr = metaValue.try_cast<glm::vec4>())
        return *ptr;

    // glm::quat
    if (auto ptr = metaValue.try_cast<glm::quat>())
        return *ptr;

    // Type not supported
    return std::nullopt;
}

entt::meta_any ComponentRegistry::PropertyValueToMetaAny(
    const SerializedPropertyValue& value,
    entt::meta_type targetType)
{
    // Try to convert variant to meta_any
    return std::visit([&](auto&& val) -> entt::meta_any {
        return entt::meta_any{val};
    }, value);
}
