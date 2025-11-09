#include "System/PrefabSystem.hpp"
#include "Component/PrefabComponent.hpp"
#include "Component/RelationshipComponent.hpp"
#include "Component/Transform.hpp"
#include "Scene/SceneGraph.hpp"
#include "Ecs/internal/reflection.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <glm/ext.hpp>
#include <entt/entt.hpp>

// Static member initialization
std::unordered_map<std::string, PrefabData> PrefabSystem::s_PrefabCache;

// ========================
// System Lifecycle
// ========================

void PrefabSystem::Init()
{
    // Initialize prefab system
    s_PrefabCache.clear();
}

void PrefabSystem::Update(ecs::world& world, float/* deltaTime*/)
{
    // Prefab system is primarily event-driven
    // No per-frame updates needed currently
}

void PrefabSystem::Exit()
{
    // Cleanup
    s_PrefabCache.clear();
}

// ========================
// Helper: YAML Serialization
// ========================

namespace {

// Convert SerializedPropertyValue to YAML node
YAML::Node SerializePropertyValue(const SerializedPropertyValue& value)
{
    YAML::Node node;

    if (std::holds_alternative<bool>(value))
        node = std::get<bool>(value);
    else if (std::holds_alternative<int>(value))
        node = std::get<int>(value);
    else if (std::holds_alternative<float>(value))
        node = std::get<float>(value);
    else if (std::holds_alternative<double>(value))
        node = std::get<double>(value);
    else if (std::holds_alternative<std::string>(value))
        node = std::get<std::string>(value);
    else if (std::holds_alternative<glm::vec2>(value)) {
        const auto& v = std::get<glm::vec2>(value);
        node.push_back(v.x);
        node.push_back(v.y);
    }
    else if (std::holds_alternative<glm::vec3>(value)) {
        const auto& v = std::get<glm::vec3>(value);
        node.push_back(v.x);
        node.push_back(v.y);
        node.push_back(v.z);
    }
    else if (std::holds_alternative<glm::vec4>(value)) {
        const auto& v = std::get<glm::vec4>(value);
        node.push_back(v.x);
        node.push_back(v.y);
        node.push_back(v.z);
        node.push_back(v.w);
    }
    else if (std::holds_alternative<glm::quat>(value)) {
        const auto& v = std::get<glm::quat>(value);
        node.push_back(v.x);
        node.push_back(v.y);
        node.push_back(v.z);
        node.push_back(v.w);
    }

    return node;
}

// Convert YAML node to SerializedPropertyValue (basic implementation)
SerializedPropertyValue DeserializePropertyValue(const YAML::Node& node, const std::string& typeName)
{
    if (typeName == "bool")
        return node.as<bool>();
    else if (typeName == "int")
        return node.as<int>();
    else if (typeName == "float")
        return node.as<float>();
    else if (typeName == "double")
        return node.as<double>();
    else if (typeName == "string")
        return node.as<std::string>();
    else if (typeName == "vec2") {
        return glm::vec2(node[0].as<float>(), node[1].as<float>());
    }
    else if (typeName == "vec3") {
        return glm::vec3(node[0].as<float>(), node[1].as<float>(), node[2].as<float>());
    }
    else if (typeName == "vec4") {
        return glm::vec4(node[0].as<float>(), node[1].as<float>(), node[2].as<float>(), node[3].as<float>());
    }
    else if (typeName == "quat") {
        return glm::quat(node[3].as<float>(), node[0].as<float>(), node[1].as<float>(), node[2].as<float>());
    }

    return 0; // Default fallback
}

// Serialize component
YAML::Node SerializeComponent(const SerializedComponent& comp)
{
    YAML::Node node;
    node["typeHash"] = comp.typeHash;
    node["typeName"] = comp.typeName;

    YAML::Node properties;
    for (const auto& [propName, propValue] : comp.properties)
    {
        properties[propName] = SerializePropertyValue(propValue);
    }
    node["properties"] = properties;

    return node;
}

// Deserialize component
SerializedComponent DeserializeComponent(const YAML::Node& node)
{
    SerializedComponent comp;
    comp.typeHash = node["typeHash"].as<std::uint32_t>(0);
    comp.typeName = node["typeName"].as<std::string>("");

    if (node["properties"])
    {
        for (const auto& prop : node["properties"])
        {
            std::string propName = prop.first.as<std::string>();
            std::string typeName = prop.second["_type"].IsDefined() ? prop.second["_type"].as<std::string>() : "float";
            comp.properties[propName] = DeserializePropertyValue(prop.second, typeName);
        }
    }

    return comp;
}

// Recursively serialize entity hierarchy
YAML::Node SerializeEntityHierarchyHelper(const PrefabEntity& entity)
{
    YAML::Node node;

    // Serialize components
    YAML::Node components;
    for (const auto& comp : entity.components)
    {
        components.push_back(SerializeComponent(comp));
    }
    node["components"] = components;

    // Recursively serialize children
    if (!entity.children.empty())
    {
        YAML::Node children;
        for (const auto& child : entity.children)
        {
            children.push_back(SerializeEntityHierarchyHelper(child));
        }
        node["children"] = children;
    }

    return node;
}

// Recursively deserialize entity hierarchy
PrefabEntity DeserializeEntityHierarchyHelper(const YAML::Node& node)
{
    PrefabEntity entity;

    // Deserialize components
    if (node["components"])
    {
        for (const auto& compNode : node["components"])
        {
            entity.components.push_back(DeserializeComponent(compNode));
        }
    }

    // Recursively deserialize children
    if (node["children"])
    {
        for (const auto& childNode : node["children"])
        {
            entity.children.push_back(DeserializeEntityHierarchyHelper(childNode));
        }
    }

    return entity;
}

// Get current timestamp in ISO 8601 format
std::string GetCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

// ========================
// Reflection Bridge Functions
// ========================

/**
 * @brief Convert entt::meta_any to SerializedPropertyValue
 * Supports: bool, int, float, double, string, vec2/3/4, quat
 */
SerializedPropertyValue MetaAnyToPropertyValue(const entt::meta_any& value)
{
    // Try each supported type
    if (auto* v = value.try_cast<bool>())
        return *v;
    if (auto* v = value.try_cast<int>())
        return *v;
    if (auto* v = value.try_cast<float>())
        return *v;
    if (auto* v = value.try_cast<double>())
        return *v;
    if (auto* v = value.try_cast<std::string>())
        return *v;
    if (auto* v = value.try_cast<glm::vec2>())
        return *v;
    if (auto* v = value.try_cast<glm::vec3>())
        return *v;
    if (auto* v = value.try_cast<glm::vec4>())
        return *v;
    if (auto* v = value.try_cast<glm::quat>())
        return *v;

    // Unsupported type - return default int
    return 0;
}

/**
 * @brief Convert SerializedPropertyValue to entt::meta_any
 */
entt::meta_any PropertyValueToMetaAny(const SerializedPropertyValue& value)
{
    return std::visit([](auto&& arg) -> entt::meta_any {
        return entt::forward_as_meta(arg);
    }, value);
}

/**
 * @brief Serialize a component using reflection
 * @param componentPtr Pointer to component data
 * @param metaType Reflection meta_type for the component
 * @return SerializedComponent with all reflected properties
 */
SerializedComponent SerializeComponentViaReflection(const void* componentPtr, const entt::meta_type& metaType)
{
    // Create meta_any from raw pointer
    entt::meta_any componentAny = metaType.from_void(componentPtr);

    // Get type information from reflection
    std::uint32_t typeHash = static_cast<std::uint32_t>(metaType.id());
    std::string typeName = ReflectionRegistry::GetTypeName(metaType.id());

    // Create serialized component with reflection type info
    SerializedComponent serialized(typeHash, typeName);

    // Get field name mapping
    auto fieldNames = ReflectionRegistry::GetFieldNames(metaType.id());

    // Iterate all reflected fields
    for (const auto& [fieldId, fieldData] : metaType.data())
    {
        // Get field name
        auto fieldNameIt = fieldNames.find(fieldId);
        if (fieldNameIt == fieldNames.end())
            continue;  // Skip if no name registered

        std::string fieldName = fieldNameIt->second;

        // Get field value
        entt::meta_any fieldValue = fieldData.get(componentAny);

        // Convert to PropertyValue
        SerializedPropertyValue propValue = MetaAnyToPropertyValue(fieldValue);

        // Store in serialized component
        serialized.properties[fieldName] = propValue;
    }

    return serialized;
}

/**
 * @brief Apply serialized component data to entity using reflection
 * @param registry ECS registry
 * @param enttEntity EnTT entity handle
 * @param serialized Serialized component data
 * @return true if component was successfully applied
 */
bool DeserializeComponentViaReflection(entt::registry& registry, entt::entity enttEntity,
                                       const SerializedComponent& serialized)
{
    // Get type name from serialized component
    if (serialized.typeName.empty())
        return false;  // No type name stored

    // Find meta_type by name
    entt::meta_type metaType;
    for (const auto& [typeHash, mt] : ReflectionRegistry::types())
    {
        if (ReflectionRegistry::GetTypeName(typeHash) == serialized.typeName)
        {
            metaType = mt;
            break;
        }
    }

    if (!metaType)
        return false;  // Type not found in reflection registry

    // Construct component instance
    entt::meta_any componentAny = metaType.construct();
    if (!componentAny)
        return false;  // Failed to construct

    // Get field name mapping
    auto fieldNames = ReflectionRegistry::GetFieldNames(metaType.id());

    // Set each property from serialized data
    for (const auto& [fieldId, fieldData] : metaType.data())
    {
        // Get field name
        auto fieldNameIt = fieldNames.find(fieldId);
        if (fieldNameIt == fieldNames.end())
            continue;

        std::string fieldName = fieldNameIt->second;

        // Check if property exists in serialized data
        auto propIt = serialized.properties.find(fieldName);
        if (propIt == serialized.properties.end())
            continue;  // Property not in serialized data

        // Convert PropertyValue to meta_any
        entt::meta_any propValue = PropertyValueToMetaAny(propIt->second);

        // Set field value
        fieldData.set(componentAny, propValue);
    }

    // Add component to entity using reflection's emplace function
    auto emplaceFunc = metaType.func("emplace_meta_any"_tn);
    if (emplaceFunc)
    {
        emplaceFunc.invoke({}, entt::forward_as_meta(registry), enttEntity, entt::forward_as_meta(componentAny));
        return true;
    }

    return false;
}

} // anonymous namespace

// ========================
// Prefab Data Management
// ========================

PrefabData PrefabSystem::LoadPrefabFromFile(const std::string& prefabPath)
{
    PrefabData data;

    try
    {
        YAML::Node root = YAML::LoadFile(prefabPath);

        // Load metadata
        if (root["metadata"])
        {
            const auto& meta = root["metadata"];
            data.name = meta["name"].as<std::string>();
            data.version = meta["version"].as<int>(1);
            data.createdDate = meta["created"].as<std::string>("");

            // Parse UUID from string
            std::string uuidStr = meta["uuid"].as<std::string>();
            data.uuid = UUID<128>::FromString(uuidStr);
        }

        // Load entity hierarchy
        if (root["root"])
        {
            data.root = DeserializeEntityHierarchyHelper(root["root"]);
        }
    }
    catch (const std::exception& e)
    {
        // Log error
        data.name = ""; // Mark as invalid
    }

    return data;
}

bool PrefabSystem::SavePrefabToFile(const PrefabData& data, const std::string& prefabPath)
{
    if (!data.IsValid())
        return false;

    try
    {
        YAML::Node root;

        // Serialize metadata
        YAML::Node metadata;
        metadata["uuid"] = data.GetUuidString();
        metadata["name"] = data.name;
        metadata["version"] = data.version;
        metadata["created"] = data.createdDate.empty() ? GetCurrentTimestamp() : data.createdDate;
        root["metadata"] = metadata;

        // Serialize entity hierarchy
        root["root"] = SerializeEntityHierarchyHelper(data.root);

        // Write to file
        std::ofstream file(prefabPath);
        if (!file.is_open())
            return false;

        file << root;
        file.close();

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

// ========================
// Core Prefab Operations
// ========================

ecs::entity PrefabSystem::InstantiatePrefab(ecs::world& world, const UUID<128>& prefabId, const glm::vec3& position)
{
    // Load prefab data from cache
    UUID<128> mutableId = prefabId;  // Make a mutable copy
    std::string uuidStr = mutableId.ToString();
    auto it = s_PrefabCache.find(uuidStr);
    if (it == s_PrefabCache.end())
    {
        // Prefab not in cache
        // TODO: Need file path to load from disk
        // For now, return invalid entity
        return ecs::entity();
    }

    const PrefabData& prefabData = it->second;

    // Create root entity and hierarchy
    ecs::entity invalidParent;  // Invalid parent for root
    ecs::entity rootEntity = InstantiateEntity(world, prefabData.root, invalidParent, prefabId);

    // Attach PrefabComponent to root
    rootEntity.add<PrefabComponent>(prefabId);

    // Apply position to root entity's transform
    if (rootEntity.all<TransformComponent>())
    {
        auto& transform = rootEntity.get<TransformComponent>();
        transform.m_Translation = position;
    }

    return rootEntity;
}

ecs::entity PrefabSystem::InstantiatePrefabWithId(ecs::world& world, const UUID<128>& prefabId, ecs::entity rootEntityId, const glm::vec3& position)
{
    // Similar to InstantiatePrefab but uses provided entity ID
    return rootEntityId;
}

int PrefabSystem::SyncPrefab(ecs::world& world, const UUID<128>& prefabId)
{
    int syncCount = 0;

    // Find all instances
    auto instances = GetAllInstances(world, prefabId);

    // Sync each instance
    for (auto instance : instances)
    {
        if (SyncInstance(world, instance))
            syncCount++;
    }

    return syncCount;
}

bool PrefabSystem::SyncInstance(ecs::world& world, ecs::entity instance)
{
    // Check if entity has PrefabComponent
    if (!instance.all<PrefabComponent>())
        return false;

    // Get PrefabComponent
    auto& prefabComp = instance.get<PrefabComponent>();

    // Load prefab data from cache
    UUID<128> mutableGuid = prefabComp.m_PrefabGuid;
    std::string uuidStr = mutableGuid.ToString();
    auto it = s_PrefabCache.find(uuidStr);
    if (it == s_PrefabCache.end())
        return false;  // Prefab not in cache

    const PrefabData& prefabData = it->second;

    // Apply prefab data while preserving overrides
    ApplyPrefabDataToEntity(world, instance, prefabData.root, &prefabComp);

    return true;
}

// ========================
// Override Management
// ========================

void PrefabSystem::MarkPropertyOverridden(ecs::world& world, ecs::entity instance,
                                          std::uint32_t componentTypeHash,
                                          const std::string& componentTypeName,
                                          const std::string& propertyPath,
                                          PropertyValue value)
{
    // Check and get PrefabComponent
    if (instance.all<PrefabComponent>())
    {
        auto& prefabComp = instance.get<PrefabComponent>();
        prefabComp.SetPropertyOverride(componentTypeHash, componentTypeName, propertyPath, value);
    }
}

bool PrefabSystem::RevertOverride(ecs::world& world, ecs::entity instance,
                                  std::uint32_t componentTypeHash,
                                  const std::string& propertyPath)
{
    // Check if entity has PrefabComponent
    if (!instance.all<PrefabComponent>())
        return false;

    // Get PrefabComponent
    auto& prefabComp = instance.get<PrefabComponent>();

    // Remove override
    bool removed = prefabComp.RemovePropertyOverride(componentTypeHash, propertyPath);

    if (removed)
    {
        // Reload property from prefab
        SyncInstance(world, instance);
    }

    return removed;
}

void PrefabSystem::RevertAllOverrides(ecs::world& world, ecs::entity instance)
{
    // Check and get PrefabComponent
    if (instance.all<PrefabComponent>())
    {
        auto& prefabComp = instance.get<PrefabComponent>();
        prefabComp.ClearAllPropertyOverrides();
        prefabComp.m_AddedComponents.clear();
        prefabComp.m_DeletedComponents.clear();

        // Reload entire instance from prefab
        SyncInstance(world, instance);
    }
}

bool PrefabSystem::ApplyInstanceToPrefab(ecs::world& world, ecs::entity instance, const std::string& prefabPath)
{
    // Check and get PrefabComponent
    if (!instance.all<PrefabComponent>())
        return false;

    auto& prefabComp = instance.get<PrefabComponent>();

    // Serialize entity hierarchy
    PrefabData prefabData;
    prefabData.uuid = prefabComp.m_PrefabGuid;
    prefabData.root = SerializeEntityHierarchy(world, instance);

    // Get name from cache if available
    std::string uuidStr = prefabData.GetUuidString();
    auto it = s_PrefabCache.find(uuidStr);
    if (it != s_PrefabCache.end())
    {
        prefabData.name = it->second.name;
        prefabData.createdDate = it->second.createdDate;
    }
    else
    {
        prefabData.name = "UpdatedPrefab";
        prefabData.createdDate = GetCurrentTimestamp();
    }

    // Save to file
    if (SavePrefabToFile(prefabData, prefabPath))
    {
        // Update cache
        s_PrefabCache[uuidStr] = prefabData;
        return true;
    }

    return false;
}

// ========================
// Queries
// ========================

std::vector<ecs::entity> PrefabSystem::GetAllInstances(ecs::world& world, const UUID<128>& prefabId)
{
    std::vector<ecs::entity> instances;

    // Query all entities with PrefabComponent
    auto entities = world.filter_entities<PrefabComponent>();
    for (auto entity : entities)
    {
        auto& prefabComp = entity.get<PrefabComponent>();
        if (prefabComp.m_PrefabGuid == prefabId)
        {
            instances.push_back(entity);
        }
    }

    return instances;
}

bool PrefabSystem::IsInstanceOf(ecs::world& world, ecs::entity entity, const UUID<128>& prefabId)
{
    if (!entity.all<PrefabComponent>())
        return false;

    auto& prefabComp = entity.get<PrefabComponent>();
    return prefabComp.m_PrefabGuid == prefabId;
}

bool PrefabSystem::IsPrefabInstance(ecs::world& world, ecs::entity entity)
{
    return entity.all<PrefabComponent>();
}

UUID<128> PrefabSystem::CreatePrefabFromEntity(ecs::world& world, ecs::entity rootEntity,
                                               const std::string& prefabName,
                                               const std::string& prefabPath)
{
    PrefabData prefabData(prefabName);
    prefabData.createdDate = GetCurrentTimestamp();

    // Serialize entity hierarchy
    prefabData.root = SerializeEntityHierarchy(world, rootEntity);

    // Save to file
    if (SavePrefabToFile(prefabData, prefabPath))
    {
        // Cache the prefab
        s_PrefabCache[prefabData.GetUuidString()] = prefabData;
        return prefabData.uuid;
    }

    return UUID<128>(); // Invalid UUID on failure
}

// ========================
// Private Helper Methods
// ========================

ecs::entity PrefabSystem::InstantiateEntity(ecs::world& world, const PrefabEntity& prefabEntity,
                                            ecs::entity parent, const UUID<128>& prefabId)
{
    // Create entity
    ecs::entity entity = world.add_entity();

    // Apply component data
    for (const auto& compData : prefabEntity.components)
    {
        ApplyComponentData(world, entity, compData);
    }

    // Set parent if provided (check if parent is valid by trying to get a UUID)
    if (parent.get_uuid() != 0)  // Valid entity has non-zero UUID
    {
        SceneGraph::SetParent(entity, parent, false);  // false = don't keep world transform
    }

    // Recursively create children
    for (const auto& childData : prefabEntity.children)
    {
        InstantiateEntity(world, childData, entity, prefabId);
    }

    return entity;
}

void PrefabSystem::ApplyPrefabDataToEntity(ecs::world& world, ecs::entity entity,
                                           const PrefabEntity& prefabEntity,
                                           const PrefabComponent* prefabComp)
{
    // Apply each component's data, respecting overrides
    for (const auto& compData : prefabEntity.components)
    {
        // Skip if component was deleted in instance
        if (prefabComp && prefabComp->HasDeletedComponent(compData.typeHash))
            continue;

        // Apply component data (this will create or update the component)
        ApplyComponentData(world, entity, compData);

        // TODO: Reapply property overrides after setting base values
        // This would require looking up overrides for this component type hash
        // and setting them back on the component
    }
}

PrefabEntity PrefabSystem::SerializeEntityHierarchy(ecs::world& world, ecs::entity entity)
{
    PrefabEntity prefabEntity;

    auto& registry = world.impl.get_registry();
    entt::entity enttEntity = ecs::world::detail::entt_entity_cast(entity);

    // Iterate all registered component types via reflection
    for (const auto& [typeHash, metaType] : ReflectionRegistry::types())
    {
        // Check if entity has this component
        auto* storage = registry.storage(typeHash);
        if (!storage || !storage->contains(enttEntity))
            continue;  // Entity doesn't have this component

        // Get component data pointer
        const void* componentPtr = storage->value(enttEntity);

        // Serialize using reflection
        SerializedComponent serializedComp = SerializeComponentViaReflection(componentPtr, metaType);

        prefabEntity.components.push_back(serializedComp);
    }

    // Recursively serialize children
    auto children = SceneGraph::GetChildren(entity);
    for (auto child : children)
    {
        prefabEntity.children.push_back(SerializeEntityHierarchy(world, child));
    }

    return prefabEntity;
}

void PrefabSystem::ApplyComponentData(ecs::world& world, ecs::entity entity,
                                      const SerializedComponent& componentData)
{
    // Use reflection-based deserialization
    DeserializeComponentViaReflection(world.impl.get_registry(), ecs::world::detail::entt_entity_cast(entity), componentData);
}

bool PrefabSystem::IsPropertyOverridden(const PrefabComponent* prefabComp,
                                        std::uint32_t componentTypeHash,
                                        const std::string& propertyPath)
{
    if (!prefabComp)
        return false;

    return prefabComp->GetPropertyOverride(componentTypeHash, propertyPath) != nullptr;
}
