#include "System/PrefabSystem.hpp"
#include "Component/PrefabComponent.hpp"
#include "Component/RelationshipComponent.hpp"
#include "Scene/SceneGraph.hpp"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

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

void PrefabSystem::Update(ecs::world& world, float deltaTime)
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
    node["type"] = static_cast<int>(comp.type);

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
    comp.type = static_cast<Component::ComponentType>(node["type"].as<int>());

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

            // Parse UUID
            std::string uuidStr = meta["uuid"].as<std::string>();
            // TODO: Implement UUID parsing from string
            // For now, generate a new one
            data.uuid = UUID<128>::Generate();
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
    // TODO: Load prefab data from cache or file system
    // For now, return invalid entity as placeholder

    // Create root entity
    ecs::entity rootEntity; // = world.create();

    // Attach PrefabComponent
    // rootEntity.add<PrefabComponent>(prefabId);

    // Recursively create entity hierarchy
    // InstantiateEntity(world, prefabData.root, rootEntity, prefabId);

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
    // Get PrefabComponent
    // auto* prefabComp = instance.try_get<PrefabComponent>();
    // if (!prefabComp)
    //     return false;

    // Load prefab data
    // PrefabData prefabData = LoadPrefabData(prefabComp->m_PrefabGuid);

    // Apply prefab data while preserving overrides
    // ApplyPrefabDataToEntity(world, instance, prefabData.root, prefabComp);

    return true;
}

// ========================
// Override Management
// ========================

void PrefabSystem::MarkPropertyOverridden(ecs::world& world, ecs::entity instance,
                                          Component::ComponentType componentType,
                                          const std::string& propertyPath,
                                          PropertyValue value)
{
    // Get PrefabComponent
    // auto* prefabComp = instance.try_get<PrefabComponent>();
    // if (prefabComp)
    // {
    //     prefabComp->SetPropertyOverride(componentType, propertyPath, value);
    // }
}

bool PrefabSystem::RevertOverride(ecs::world& world, ecs::entity instance,
                                  Component::ComponentType componentType,
                                  const std::string& propertyPath)
{
    // Get PrefabComponent
    // auto* prefabComp = instance.try_get<PrefabComponent>();
    // if (!prefabComp)
    //     return false;

    // Remove override
    // bool removed = prefabComp->RemovePropertyOverride(componentType, propertyPath);

    // if (removed)
    // {
    //     // Reload property from prefab
    //     SyncInstance(world, instance);
    // }

    return true;
}

void PrefabSystem::RevertAllOverrides(ecs::world& world, ecs::entity instance)
{
    // Get PrefabComponent
    // auto* prefabComp = instance.try_get<PrefabComponent>();
    // if (prefabComp)
    // {
    //     prefabComp->ClearAllPropertyOverrides();
    //     prefabComp->m_AddedComponents.clear();
    //     prefabComp->m_DeletedComponents.clear();
    //
    //     // Reload entire instance from prefab
    //     SyncInstance(world, instance);
    // }
}

bool PrefabSystem::ApplyInstanceToPrefab(ecs::world& world, ecs::entity instance, const std::string& prefabPath)
{
    // Get PrefabComponent
    // auto* prefabComp = instance.try_get<PrefabComponent>();
    // if (!prefabComp)
    //     return false;

    // Serialize entity hierarchy
    // PrefabData prefabData;
    // prefabData.uuid = prefabComp->m_PrefabGuid;
    // prefabData.root = SerializeEntityHierarchy(world, instance);

    // Save to file
    // return SavePrefabToFile(prefabData, prefabPath);

    return true;
}

// ========================
// Queries
// ========================

std::vector<ecs::entity> PrefabSystem::GetAllInstances(ecs::world& world, const UUID<128>& prefabId)
{
    std::vector<ecs::entity> instances;

    // Query all entities with PrefabComponent
    // world.query<PrefabComponent>([&](ecs::entity entity, PrefabComponent& prefabComp) {
    //     if (prefabComp.m_PrefabGuid == prefabId)
    //     {
    //         instances.push_back(entity);
    //     }
    // });

    return instances;
}

bool PrefabSystem::IsInstanceOf(ecs::world& world, ecs::entity entity, const UUID<128>& prefabId)
{
    // auto* prefabComp = entity.try_get<PrefabComponent>();
    // return prefabComp && prefabComp->m_PrefabGuid == prefabId;
    return false;
}

bool PrefabSystem::IsPrefabInstance(ecs::world& world, ecs::entity entity)
{
    // return entity.has<PrefabComponent>();
    return false;
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
    // ecs::entity entity = world.create();

    // Apply component data
    // for (const auto& compData : prefabEntity.components)
    // {
    //     ApplyComponentData(world, entity, compData);
    // }

    // Set parent if provided
    // if (parent.is_valid())
    // {
    //     SceneGraph::SetParent(entity, parent);
    // }

    // Recursively create children
    // for (const auto& childData : prefabEntity.children)
    // {
    //     InstantiateEntity(world, childData, entity, prefabId);
    // }

    // return entity;
    return ecs::entity();
}

void PrefabSystem::ApplyPrefabDataToEntity(ecs::world& world, ecs::entity entity,
                                           const PrefabEntity& prefabEntity,
                                           const PrefabComponent* prefabComp)
{
    // Apply each component's data, respecting overrides
    // for (const auto& compData : prefabEntity.components)
    // {
    //     // Skip if component was deleted in instance
    //     if (prefabComp && prefabComp->HasDeletedComponent(compData.type))
    //         continue;
    //
    //     ApplyComponentData(world, entity, compData);
    // }
}

PrefabEntity PrefabSystem::SerializeEntityHierarchy(ecs::world& world, ecs::entity entity)
{
    PrefabEntity prefabEntity;

    // Serialize components
    // TODO: Implement component serialization
    // This would iterate over all components and serialize their data

    // Recursively serialize children
    // auto children = SceneGraph::GetChildren(entity);
    // for (auto child : children)
    // {
    //     prefabEntity.children.push_back(SerializeEntityHierarchy(world, child));
    // }

    return prefabEntity;
}

void PrefabSystem::ApplyComponentData(ecs::world& world, ecs::entity entity,
                                      const SerializedComponent& componentData)
{
    // Apply component data based on type
    // This would use a component factory or switch statement
    // to create/update the appropriate component type
}

bool PrefabSystem::IsPropertyOverridden(const PrefabComponent* prefabComp,
                                        Component::ComponentType componentType,
                                        const std::string& propertyPath)
{
    if (!prefabComp)
        return false;

    return prefabComp->GetPropertyOverride(componentType, propertyPath) != nullptr;
}
