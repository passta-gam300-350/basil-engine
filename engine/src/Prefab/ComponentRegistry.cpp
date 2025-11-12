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

bool ComponentRegistry::DeserializeComponent(
    ecs::entity entity,
    const SerializedComponent& data)
{
    // Get the meta type
    auto& types = ReflectionRegistry::types();
    auto typeIt = types.find(data.typeHash);
    if (typeIt == types.end())
    {
        return false; // Type not registered
    }

    entt::meta_type metaType = typeIt->second;

    // TODO: This is the tricky part - we need to create/get the component on the entity
    // For now, this is a placeholder that would need to be implemented with proper
    // component construction/access via the ECS system

    // The proper implementation would require:
    // 1. Check if entity has component (entity.all<T>())
    // 2. If not, create it with default constructor (entity.add<T>())
    // 3. Get reference to component (entity.get<T>())
    // 4. Apply properties using reflection

    // This requires runtime component construction which needs more infrastructure
    // For now, we'll leave this as a TODO and implement it in the PrefabSystem
    // where we can use template specialization for known component types

    return false; // Not fully implemented yet
}

std::optional<SerializedPropertyValue> ComponentRegistry::GetProperty(
    const SerializedComponent& component,
    const std::string& propertyPath)
{
    // For now, support direct property access (not nested paths)
    // TODO: Implement nested path parsing (e.g., "m_Position.x")

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
    // For now, support direct property access (not nested paths)
    // TODO: Implement nested path parsing

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
    // TODO: Implement component removal by type hash
    // This requires runtime type dispatch which needs more infrastructure
    // For now, leave as placeholder
    return false;
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
