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
#include <spdlog/spdlog.h>
#include "Manager/ResourceSystem.hpp"

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

void PrefabSystem::Update(ecs::world& /*world*/, float/* deltaTime*/)
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
    else if (std::holds_alternative<rp::BasicIndexedGuid>(value)) {
        // Serialize GUID as nested map (matches scene format)
        const auto& guid = std::get<rp::BasicIndexedGuid>(value);
        node["guid"] = guid.m_guid.to_hex();
        node["type"] = guid.m_typeindex;
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

    // Check if this is a GUID (nested map with "guid" and "type" keys)
    if (node.IsMap() && node["guid"] && node["type"])
    {
        try
        {
            std::string guidStr = node["guid"].as<std::string>();
            std::uint64_t typeindex = node["type"].as<std::uint64_t>();

            rp::Guid guid = rp::Guid::to_guid(guidStr.c_str());
            rp::BasicIndexedGuid indexedGuid{guid, typeindex};

            return indexedGuid;
        }
        catch (const std::exception& e)
        {
            spdlog::error("Failed to deserialize GUID: {}", e.what());
        }
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

// Infer property type from YAML node structure
std::string InferPropertyType(const YAML::Node& node)
{
    if (!node.IsDefined() || node.IsNull())
        return "int"; // Default for null/undefined

    if (node.IsScalar())
    {
        // Get the raw string value to inspect
        std::string valueStr;
        try {
            valueStr = node.Scalar(); // Get raw scalar string without conversion
        } catch (...) {
            return "string";
        }

        if (valueStr.empty())
            return "string";

        // Check for boolean literals
        if (valueStr == "true" || valueStr == "false")
            return "bool";

        // Check if it's a number
        bool hasDigit = false;
        bool hasDot = false;
        bool hasE = false;
        bool isNegative = (valueStr[0] == '-');
        size_t startIdx = isNegative ? 1 : 0;

        if (startIdx >= valueStr.length())
            return "string"; // Just a minus sign

        for (size_t i = startIdx; i < valueStr.length(); ++i)
        {
            char c = valueStr[i];
            if (std::isdigit(c))
                hasDigit = true;
            else if (c == '.')
                hasDot = true;
            else if (c == 'e' || c == 'E')
                hasE = true;
            else
                return "string"; // Non-numeric character found
        }

        if (!hasDigit)
            return "string"; // No digits found

        if (hasDot || hasE)
            return "float";
        else
            return "int";
    }
    else if (node.IsSequence())
    {
        // Determine vector type by size
        size_t size = node.size();
        if (size == 2)
            return "vec2";
        else if (size == 3)
            return "vec3";
        else if (size == 4)
            return "vec4"; // Could be vec4 or quat, default to vec4

        return "float"; // Fallback
    }

    return "float"; // Default fallback
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

            // Infer type from YAML structure instead of expecting "_type" field
            std::string typeName = InferPropertyType(prop.second);

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

    // === NESTED PREFAB SUPPORT ===
    // Serialize nested prefab references
    if (!entity.nestedPrefabs.empty())
    {
        YAML::Node nestedPrefabs;
        for (const auto& nestedRef : entity.nestedPrefabs)
        {
            YAML::Node nestedNode;
            nestedNode["guid"] = nestedRef.prefabGuid.m_guid.to_hex();
            nestedNode["typeIndex"] = nestedRef.prefabGuid.m_typeindex;
            nestedNode["name"] = nestedRef.prefabName;

            // Serialize transform overrides
            YAML::Node position;
            position.push_back(nestedRef.positionOverride.x);
            position.push_back(nestedRef.positionOverride.y);
            position.push_back(nestedRef.positionOverride.z);
            nestedNode["positionOverride"] = position;

            YAML::Node rotation;
            rotation.push_back(nestedRef.rotationOverride.x);
            rotation.push_back(nestedRef.rotationOverride.y);
            rotation.push_back(nestedRef.rotationOverride.z);
            rotation.push_back(nestedRef.rotationOverride.w);
            nestedNode["rotationOverride"] = rotation;

            YAML::Node scale;
            scale.push_back(nestedRef.scaleOverride.x);
            scale.push_back(nestedRef.scaleOverride.y);
            scale.push_back(nestedRef.scaleOverride.z);
            nestedNode["scaleOverride"] = scale;

            // Serialize component overrides
            if (!nestedRef.componentOverrides.empty())
            {
                YAML::Node overrides;
                for (const auto& overrideComp : nestedRef.componentOverrides)
                {
                    overrides.push_back(SerializeComponent(overrideComp));
                }
                nestedNode["componentOverrides"] = overrides;
            }

            nestedPrefabs.push_back(nestedNode);
        }
        node["nestedPrefabs"] = nestedPrefabs;
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

    // === NESTED PREFAB SUPPORT ===
    // Deserialize nested prefab references
    if (node["nestedPrefabs"])
    {
        for (const auto& nestedNode : node["nestedPrefabs"])
        {
            NestedPrefabReference nestedRef;

            // Deserialize GUID
            std::string guidStr = nestedNode["guid"].as<std::string>("");
            std::uint64_t typeIndex = nestedNode["typeIndex"].as<std::uint64_t>(0);

            // Parse GUID from hex string
            try {
                rp::Guid guid = rp::Guid::to_guid(guidStr.c_str());
                nestedRef.prefabGuid = rp::BasicIndexedGuid{guid, typeIndex};
            }
            catch (const std::exception& e)
            {
                spdlog::error("Failed to parse nested prefab GUID: {} - {}", guidStr, e.what());
                continue; // Skip this nested prefab
            }

            nestedRef.prefabName = nestedNode["name"].as<std::string>("Unknown");

            // Deserialize transform overrides
            if (nestedNode["positionOverride"] && nestedNode["positionOverride"].size() == 3)
            {
                nestedRef.positionOverride = glm::vec3(
                    nestedNode["positionOverride"][0].as<float>(),
                    nestedNode["positionOverride"][1].as<float>(),
                    nestedNode["positionOverride"][2].as<float>()
                );
            }

            if (nestedNode["rotationOverride"] && nestedNode["rotationOverride"].size() == 4)
            {
                nestedRef.rotationOverride = glm::quat(
                    nestedNode["rotationOverride"][3].as<float>(), // w
                    nestedNode["rotationOverride"][0].as<float>(), // x
                    nestedNode["rotationOverride"][1].as<float>(), // y
                    nestedNode["rotationOverride"][2].as<float>()  // z
                );
            }

            if (nestedNode["scaleOverride"] && nestedNode["scaleOverride"].size() == 3)
            {
                nestedRef.scaleOverride = glm::vec3(
                    nestedNode["scaleOverride"][0].as<float>(),
                    nestedNode["scaleOverride"][1].as<float>(),
                    nestedNode["scaleOverride"][2].as<float>()
                );
            }

            // Deserialize component overrides
            if (nestedNode["componentOverrides"])
            {
                for (const auto& overrideNode : nestedNode["componentOverrides"])
                {
                    nestedRef.componentOverrides.push_back(DeserializeComponent(overrideNode));
                }
            }

            entity.nestedPrefabs.push_back(nestedRef);
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

    // Check for rp::BasicIndexedGuid - return directly, not as string
    if (auto* guid = value.try_cast<rp::BasicIndexedGuid>())
        return *guid;

    // Unsupported type - return default int
    spdlog::warn("MetaAnyToPropertyValue: Unsupported type encountered (type_id: {})", value.type().id());
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
    // The serialized typeHash is hash(typeName), stored in m_Names
    // Try multiple strategies to find the correct meta_type
    entt::meta_type metaType;
    auto& reg = ReflectionRegistry::Registry();

    // Strategy 1: Try direct lookup in m_Storage using serialized.typeHash
    // (in case custom hash is also used as storage key)
    spdlog::debug("DeserializeComponent '{}': Trying Strategy 1 (m_Storage lookup with hash {})",
                 serialized.typeName, serialized.typeHash);
    auto storageIt = reg.m_Storage.find(serialized.typeHash);
    if (storageIt != reg.m_Storage.end())
    {
        spdlog::info("Strategy 1 SUCCESS: Found '{}' in m_Storage", serialized.typeName);
        metaType = storageIt->second;
    }
    else
    {
        spdlog::debug("Strategy 1 failed: '{}' not found in m_Storage", serialized.typeName);
    }

    // Strategy 2: Resolve by registered type name using ToTypeName hash
    if (!metaType)
    {
        spdlog::debug("DeserializeComponent '{}': Trying Strategy 2 (entt::resolve with ToTypeName)",
                     serialized.typeName);
        auto resolvedType = entt::resolve(ToTypeName(serialized.typeName));
        if (resolvedType)
        {
            spdlog::info("Strategy 2 SUCCESS: entt::resolve found '{}'", serialized.typeName);
            metaType = resolvedType;
        }
        else
        {
            spdlog::debug("Strategy 2 failed: entt::resolve returned empty for '{}'", serialized.typeName);
        }
    }

    // Strategy 3: Search m_Storage by checking if the component can be found in registry
    if (!metaType)
    {
        for (const auto& [hash, mt] : reg.m_Storage)
        {
            if (!mt)
                continue;

            // Try to match by checking if this hash works with registry.storage()
            // and checking the serialized.typeName
            // This is a last resort - just try the first match we find
            auto namesIt = reg.m_Names.find(serialized.typeHash);
            if (namesIt != reg.m_Names.end() && namesIt->second == serialized.typeName)
            {
                // TypeHash is valid in m_Names, try this meta_type
                metaType = mt;
                break;
            }
        }
    }

    if (!metaType)
    {
        spdlog::error("Component '{}' (typeHash: {}) not found in reflection registry - ALL STRATEGIES FAILED",
                     serialized.typeName, serialized.typeHash);
        return false;
    }

    // Construct component instance
    spdlog::debug("DeserializeComponent '{}': Attempting to construct component instance", serialized.typeName);
    entt::meta_any componentAny = metaType.construct();
    if (!componentAny)
    {
        spdlog::error("FAILED to construct component '{}' - metaType.construct() returned empty",
                     serialized.typeName);
        return false;  // Failed to construct
    }
    spdlog::debug("Component '{}' constructed successfully", serialized.typeName);

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

    // Add or replace component to entity using reflection (handles both new and existing components)
    spdlog::debug("DeserializeComponent '{}': Looking for 'emplace_or_replace_meta_any' function", serialized.typeName);
    auto emplaceFunc = metaType.func("emplace_or_replace_meta_any"_tn);
    if (emplaceFunc)
    {
        spdlog::debug("Component '{}': Invoking emplace_or_replace_meta_any", serialized.typeName);
        emplaceFunc.invoke({}, entt::forward_as_meta(registry), enttEntity, entt::forward_as_meta(componentAny));
        spdlog::debug("Component '{}' successfully emplaced to entity", serialized.typeName);

        // Auto-add TransformMtxComponent when TransformComponent is added
        // (mimics the behavior in world::add_component_to_entity at world.h:179-180)
        if (serialized.typeName == "TransformComponent")
        {
            if (!registry.all_of<TransformMtxComponent>(enttEntity))
            {
                registry.emplace<TransformMtxComponent>(enttEntity);
                spdlog::debug("Auto-created TransformMtxComponent for TransformComponent");
            }
        }

        return true;
    }

    spdlog::error("Component '{}' missing 'emplace_or_replace_meta_any' function - not properly registered?",
                 serialized.typeName);
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

            // Parse GUID from string
            std::string guidStr = meta["guid"].as<std::string>();
            data.guid.m_guid = rp::Guid::to_guid(guidStr);
            data.guid.m_typeindex = meta["typeIndex"].as<std::uint64_t>(0);
        }

        // Load entity hierarchy
        if (root["root"])
        {
            data.root = DeserializeEntityHierarchyHelper(root["root"]);
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error("Failed to load prefab from file '{}': {}", prefabPath, e.what());
        data.name = ""; // Mark as invalid
    }

    return data;
}

PrefabData PrefabSystem::LoadAndCachePrefab(const std::string& prefabPath)
{
    PrefabData data = LoadPrefabFromFile(prefabPath);

    if (data.IsValid())
    {
        std::string guidStr = data.guid.m_guid.to_hex();
        s_PrefabCache[guidStr] = data;
        spdlog::info("Loaded and cached prefab: '{}' (GUID: {})", data.name, guidStr);
    }
    else
    {
        spdlog::error("Failed to load and cache prefab from: {}", prefabPath);
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
        metadata["guid"] = data.GetGuidString();
        metadata["typeIndex"] = data.guid.m_typeindex;
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
    catch (const std::exception& /*e*/)
    {
        return false;
    }
}

// ========================
// Core Prefab Operations
// ========================

ecs::entity PrefabSystem::InstantiatePrefab(ecs::world& world, const rp::BasicIndexedGuid& prefabId, const glm::vec3& position)
{
    // Load prefab data from cache
    std::string guidStr = prefabId.m_guid.to_hex();
    auto it = s_PrefabCache.find(guidStr);

    // If not in cache, try loading from ResourceSystem
    if (it == s_PrefabCache.end())
    {
        spdlog::debug("Prefab {} not in cache, attempting to load from ResourceSystem", guidStr);

        // Try to load via ResourceSystem
        PrefabData* loadedPrefab = ResourceRegistry::Instance().Get<PrefabData>(prefabId.m_guid);

        if (loadedPrefab && loadedPrefab->IsValid())
        {
            // Add to cache for future use
            s_PrefabCache[guidStr] = *loadedPrefab;
            it = s_PrefabCache.find(guidStr);
            spdlog::info("Successfully loaded prefab '{}' from ResourceSystem", loadedPrefab->name);
        }
        else
        {
            spdlog::error("Failed to load prefab with GUID: {}", guidStr);
            return ecs::entity();
        }
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

ecs::entity PrefabSystem::InstantiatePrefabWithId(ecs::world& world, const rp::BasicIndexedGuid& prefabId, ecs::entity rootEntityId, const glm::vec3& position)
{
    // Load prefab data from cache
    std::string guidStr = prefabId.m_guid.to_hex();
    auto it = s_PrefabCache.find(guidStr);

    // If not in cache, try loading from ResourceSystem
    if (it == s_PrefabCache.end())
    {
        spdlog::debug("Prefab {} not in cache, attempting to load from ResourceSystem", guidStr);

        PrefabData* loadedPrefab = ResourceRegistry::Instance().Get<PrefabData>(prefabId.m_guid);

        if (loadedPrefab && loadedPrefab->IsValid())
        {
            s_PrefabCache[guidStr] = *loadedPrefab;
            it = s_PrefabCache.find(guidStr);
            spdlog::info("Successfully loaded prefab '{}' from ResourceSystem", loadedPrefab->name);
        }
        else
        {
            spdlog::error("Failed to load prefab with GUID: {}", guidStr);
            return ecs::entity();
        }
    }

    const PrefabData& prefabData = it->second;

    // Use provided entity as root (instead of creating new one)
    // Apply component data to root entity
    for (const auto& compData : prefabData.root.components)
    {
        ApplyComponentData(world, rootEntityId, compData);
    }

    // Attach PrefabComponent to root
    if (!rootEntityId.all<PrefabComponent>())
    {
        rootEntityId.add<PrefabComponent>(prefabId);
    }

    // Apply position to root entity's transform
    if (rootEntityId.all<TransformComponent>())
    {
        auto& transform = rootEntityId.get<TransformComponent>();
        transform.m_Translation = position;
    }

    // Recursively create children (they get new entity IDs)
    for (const auto& childData : prefabData.root.children)
    {
        InstantiateEntity(world, childData, rootEntityId, prefabId);
    }

    // === NESTED PREFAB SUPPORT ===
    // Instantiate any nested prefab references (same as in InstantiateEntity)
    for (const auto& nestedRef : prefabData.root.nestedPrefabs)
    {
        spdlog::info("Instantiating nested prefab: {} ({})",
                    nestedRef.prefabName, nestedRef.prefabGuid.m_guid.to_hex());

        // Recursively instantiate the nested prefab
        ecs::entity nestedInstance = InstantiatePrefab(world, nestedRef.prefabGuid, nestedRef.positionOverride);

        if (nestedInstance.get_uuid() != 0) // Valid entity
        {
            // Set root entity as the nested prefab's parent
            SceneGraph::SetParent(nestedInstance, rootEntityId, false);

            // Update nesting information in PrefabComponent
            if (nestedInstance.all<PrefabComponent>())
            {
                auto& nestedPrefabComp = nestedInstance.get<PrefabComponent>();
                nestedPrefabComp.SetNestingInfo(1, prefabId); // Level 1 since parent is root

                // Apply transform overrides
                if (nestedInstance.all<TransformComponent>())
                {
                    auto& transform = nestedInstance.get<TransformComponent>();
                    transform.m_Translation = nestedRef.positionOverride;
                    transform.m_Rotation = glm::eulerAngles(nestedRef.rotationOverride);
                    transform.m_Scale = nestedRef.scaleOverride;
                }

                // Apply component overrides
                for (const auto& overrideComp : nestedRef.componentOverrides)
                {
                    for (const auto& [propertyPath, value] : overrideComp.properties)
                    {
                        PropertyValue propValue = std::visit([](auto&& arg) -> PropertyValue {
                            return arg;
                        }, value);

                        nestedPrefabComp.SetPropertyOverride(
                            overrideComp.typeHash,
                            overrideComp.typeName,
                            propertyPath,
                            propValue
                        );
                    }
                }

                // Re-sync to apply overrides
                SyncInstance(world, nestedInstance);
            }
        }
    }

    return rootEntityId;
}

int PrefabSystem::SyncPrefab(ecs::world& world, const rp::BasicIndexedGuid& prefabId)
{
    int syncCount = 0;

    // === NESTED PREFAB SUPPORT: Sync nested prefabs first (inner → outer) ===
    // Load this prefab's data to check for nested prefab dependencies
    std::string guidStr = prefabId.m_guid.to_hex();
    auto cacheIt = s_PrefabCache.find(guidStr);

    if (cacheIt != s_PrefabCache.end())
    {
        // Get all nested prefab dependencies
        std::vector<rp::BasicIndexedGuid> nestedDependencies = GetNestedPrefabDependencies(prefabId);

        // Sync nested prefabs first (this ensures changes propagate from inner to outer)
        for (const auto& nestedPrefabId : nestedDependencies)
        {
            // Recursively sync nested prefab (this may sync even deeper nested prefabs)
            int nestedSyncCount = SyncPrefab(world, nestedPrefabId);
            spdlog::debug("Synced {} instances of nested prefab: {}",
                         nestedSyncCount, nestedPrefabId.m_guid.to_hex());
        }
    }

    // Find all direct instances of this prefab (not instances of nested prefabs)
    auto instances = GetAllInstances(world, prefabId);

    // Sync each instance
    for (auto instance : instances)
    {
        if (SyncInstance(world, instance))
            syncCount++;
    }

    spdlog::info("Synced {} instances of prefab: {}", syncCount, guidStr);
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
    std::string guidStr = prefabComp.m_PrefabGuid.m_guid.to_hex();
    auto it = s_PrefabCache.find(guidStr);
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

void PrefabSystem::MarkPropertyOverridden(ecs::world& /*world*/, ecs::entity instance,
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
    prefabData.guid = prefabComp.m_PrefabGuid;
    prefabData.root = SerializeEntityHierarchy(world, instance);

    // Get name from cache if available
    std::string guidStr = prefabData.GetGuidString();
    auto it = s_PrefabCache.find(guidStr);
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
        s_PrefabCache[guidStr] = prefabData;
        return true;
    }

    return false;
}

// ========================
// Queries
// ========================

std::vector<ecs::entity> PrefabSystem::GetAllInstances(ecs::world& world, const rp::BasicIndexedGuid& prefabId)
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

bool PrefabSystem::IsInstanceOf(ecs::world& /*world*/, ecs::entity entity, const rp::BasicIndexedGuid& prefabId)
{
    if (!entity.all<PrefabComponent>())
        return false;

    auto& prefabComp = entity.get<PrefabComponent>();
    return prefabComp.m_PrefabGuid == prefabId;
}

bool PrefabSystem::IsPrefabInstance(ecs::world& /*world*/, ecs::entity entity)
{
    return entity.all<PrefabComponent>();
}

rp::BasicIndexedGuid PrefabSystem::CreatePrefabFromEntity(ecs::world& world, ecs::entity rootEntity,
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
        s_PrefabCache[prefabData.GetGuidString()] = prefabData;
        return prefabData.guid;
    }

    return rp::null_indexed_guid; // Invalid GUID on failure
}

// ========================
// Private Helper Methods
// ========================

ecs::entity PrefabSystem::InstantiateEntity(ecs::world& world, const PrefabEntity& prefabEntity,
                                            ecs::entity parent, const rp::BasicIndexedGuid& prefabId)
{
    // Create entity
    ecs::entity entity = world.add_entity();

    spdlog::info("InstantiateEntity: Creating entity with {} components",
                 prefabEntity.components.size());

    // Apply component data
    for (const auto& compData : prefabEntity.components)
    {
        ApplyComponentData(world, entity, compData);
    }

    spdlog::info("InstantiateEntity: Completed applying components");

    // Set parent if provided (check if parent is valid by trying to get a UUID)
    if (parent.get_uuid() != 0)  // Valid entity has non-zero UUID
    {
        SceneGraph::SetParent(entity, parent, false);  // false = don't keep world transform
    }

    // Recursively create children (regular child entities)
    for (const auto& childData : prefabEntity.children)
    {
        InstantiateEntity(world, childData, entity, prefabId);
    }

    // === NESTED PREFAB SUPPORT ===
    // Instantiate any nested prefab references
    for (const auto& nestedRef : prefabEntity.nestedPrefabs)
    {
        spdlog::info("Instantiating nested prefab: {} ({})",
                    nestedRef.prefabName, nestedRef.prefabGuid.m_guid.to_hex());

        // Recursively instantiate the nested prefab
        ecs::entity nestedInstance = InstantiatePrefab(world, nestedRef.prefabGuid, nestedRef.positionOverride);

        if (nestedInstance.get_uuid() != 0) // Valid entity
        {
            // Set this entity as the nested prefab's parent
            SceneGraph::SetParent(nestedInstance, entity, false);

            // Update nesting information in PrefabComponent
            if (nestedInstance.all<PrefabComponent>())
            {
                auto& nestedPrefabComp = nestedInstance.get<PrefabComponent>();

                // Calculate nesting level (parent's level + 1, or 1 if parent doesn't have PrefabComponent)
                int parentNestingLevel = 0;
                if (parent.get_uuid() != 0 && parent.all<PrefabComponent>())
                {
                    parentNestingLevel = parent.get<PrefabComponent>().m_NestingLevel;
                }

                nestedPrefabComp.SetNestingInfo(parentNestingLevel + 1, prefabId);

                // Apply transform overrides from the nested prefab reference
                if (nestedInstance.all<TransformComponent>())
                {
                    auto& transform = nestedInstance.get<TransformComponent>();
                    transform.m_Translation = nestedRef.positionOverride;
                    transform.m_Rotation = glm::eulerAngles(nestedRef.rotationOverride);
                    transform.m_Scale = nestedRef.scaleOverride;
                }

                // Apply component overrides from the NestedPrefabReference
                for (const auto& overrideComp : nestedRef.componentOverrides)
                {
                    // Each property in this component is an override
                    for (const auto& [propertyPath, value] : overrideComp.properties)
                    {
                        // Convert SerializedPropertyValue to PropertyValue
                        PropertyValue propValue = std::visit([](auto&& arg) -> PropertyValue {
                            return arg;
                        }, value);

                        // Mark this property as overridden
                        nestedPrefabComp.SetPropertyOverride(
                            overrideComp.typeHash,
                            overrideComp.typeName,
                            propertyPath,
                            propValue
                        );

                        spdlog::debug("Applied nested prefab override: {}.{}", overrideComp.typeName, propertyPath);
                    }
                }

                // Re-sync the nested instance to apply the overrides
                SyncInstance(world, nestedInstance);
            }
        }
        else
        {
            spdlog::error("Failed to instantiate nested prefab: {} ({})",
                         nestedRef.prefabName, nestedRef.prefabGuid.m_guid.to_hex());
        }
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

        // Reapply property overrides after setting base values
        if (prefabComp && !prefabComp->m_OverriddenProperties.empty())
        {
            // Find overrides for this component type
            for (const auto& override : prefabComp->m_OverriddenProperties)
            {
                if (override.componentTypeHash == compData.typeHash)
                {
                    // Get the component via reflection
                    auto& registry = world.impl.get_registry();
                    entt::entity enttEntity = ecs::world::detail::entt_entity_cast(entity);
                    auto* storage = registry.storage(compData.typeHash);

                    if (storage && storage->contains(enttEntity))
                    {
                        void* componentPtr = const_cast<void*>(storage->value(enttEntity));

                        // Get the meta type for reflection
                        auto typeIt = ReflectionRegistry::types().find(compData.typeHash);
                        if (typeIt != ReflectionRegistry::types().end())
                        {
                            auto metaType = typeIt->second;

                            // Use reflection to set the property value
                            auto metaHandle = metaType.from_void(componentPtr);
                            if (metaHandle)
                            {
                                auto metaData = metaHandle.type().data(entt::hashed_string(override.propertyPath.c_str()));
                                if (metaData)
                                {
                                    // Convert PropertyValue to meta_any and set it
                                    std::visit([&](auto&& value) {
                                        using T = std::decay_t<decltype(value)>;
                                        if constexpr (std::is_same_v<T, float>) {
                                            metaData.set(metaHandle, value);
                                        } else if constexpr (std::is_same_v<T, int>) {
                                            metaData.set(metaHandle, value);
                                        } else if constexpr (std::is_same_v<T, bool>) {
                                            metaData.set(metaHandle, value);
                                        } else if constexpr (std::is_same_v<T, std::string>) {
                                            metaData.set(metaHandle, value);
                                        } else if constexpr (std::is_same_v<T, glm::vec3>) {
                                            metaData.set(metaHandle, value);
                                        } else if constexpr (std::is_same_v<T, glm::vec4>) {
                                            metaData.set(metaHandle, value);
                                        }
                                    }, override.value);
                                }
                            }
                        }
                    }
                }
            }
        }
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
    // IMPORTANT: Check if child is a prefab instance - if so, store as NestedPrefabReference
    auto children = SceneGraph::GetChildren(entity);
    for (auto child : children)
    {
        // Check if child is a prefab instance (has PrefabComponent)
        if (child.all<PrefabComponent>())
        {
            const auto& prefabComp = child.get<PrefabComponent>();

            // Only store as nested prefab reference if it's a ROOT prefab instance
            // (not a child of another prefab instance)
            if (prefabComp.IsRootPrefabInstance())
            {
                // Create nested prefab reference instead of expanding
                NestedPrefabReference nestedRef;
                nestedRef.prefabGuid = prefabComp.m_PrefabGuid;

                // Get prefab name from cache (if available)
                std::string guidStr = prefabComp.m_PrefabGuid.m_guid.to_hex();
                auto cacheIt = s_PrefabCache.find(guidStr);
                if (cacheIt != s_PrefabCache.end())
                {
                    nestedRef.prefabName = cacheIt->second.name;
                }
                else
                {
                    nestedRef.prefabName = "Unknown_Prefab";
                }

                // Store transform overrides (position/rotation/scale relative to parent)
                if (child.all<TransformComponent>())
                {
                    const auto& transform = child.get<TransformComponent>();
                    nestedRef.positionOverride = transform.m_Translation;
                    nestedRef.rotationOverride = glm::quat(transform.m_Rotation);
                    nestedRef.scaleOverride = transform.m_Scale;
                }

                // Store component overrides (properties that differ from the nested prefab)
                // For now, we store all overrides - in the future, we could optimize by comparing
                for (const auto& override : prefabComp.m_OverriddenProperties)
                {
                    // Find or create SerializedComponent for this component type
                    SerializedComponent* overrideComp = nullptr;
                    for (auto& comp : nestedRef.componentOverrides)
                    {
                        if (comp.typeHash == override.componentTypeHash)
                        {
                            overrideComp = &comp;
                            break;
                        }
                    }

                    if (!overrideComp)
                    {
                        // Create new SerializedComponent for this component type
                        SerializedComponent newComp(override.componentTypeHash, override.componentTypeName);
                        nestedRef.componentOverrides.push_back(newComp);
                        overrideComp = &nestedRef.componentOverrides.back();
                    }

                    // Convert PropertyValue to SerializedPropertyValue
                    SerializedPropertyValue serializedValue = std::visit([](auto&& arg) -> SerializedPropertyValue {
                        return arg;
                    }, override.value);

                    overrideComp->SetProperty(override.propertyPath, serializedValue);
                }

                // Add to nested prefabs list
                prefabEntity.nestedPrefabs.push_back(nestedRef);

                spdlog::info("Serialized nested prefab reference: {} ({})",
                            nestedRef.prefabName, nestedRef.prefabGuid.m_guid.to_hex());
            }
            else
            {
                // This is a child OF a prefab instance - serialize normally (shouldn't happen in prefab creation)
                spdlog::warn("Found nested prefab instance inside another prefab instance - expanding fully");
                prefabEntity.children.push_back(SerializeEntityHierarchy(world, child));
            }
        }
        else
        {
            // Regular child entity - serialize recursively
            prefabEntity.children.push_back(SerializeEntityHierarchy(world, child));
        }
    }

    return prefabEntity;
}

void PrefabSystem::ApplyComponentData(ecs::world& world, ecs::entity entity,
                                      const SerializedComponent& componentData)
{
    // Use reflection-based deserialization
    bool success = DeserializeComponentViaReflection(world.impl.get_registry(),
                                                      ecs::world::detail::entt_entity_cast(entity),
                                                      componentData);

    if (!success)
    {
        spdlog::error("FAILED to apply component '{}' (typeHash: {}) to entity",
                     componentData.typeName, componentData.typeHash);
    }
    else
    {
        spdlog::info("Successfully applied component '{}'", componentData.typeName);
    }
}

bool PrefabSystem::IsPropertyOverridden(const PrefabComponent* prefabComp,
                                        std::uint32_t componentTypeHash,
                                        const std::string& propertyPath)
{
    if (!prefabComp)
        return false;

    return prefabComp->GetPropertyOverride(componentTypeHash, propertyPath) != nullptr;
}

// ========================
// Nested Prefab Support
// ========================

bool PrefabSystem::DetectCircularDependency(const rp::BasicIndexedGuid& prefabId,
                                            std::vector<rp::BasicIndexedGuid>& visitedPrefabs)
{
    // Check if this prefab is already in the visited set (circular dependency!)
    for (const auto& visited : visitedPrefabs)
    {
        if (visited.m_guid == prefabId.m_guid)
        {
            spdlog::error("Circular dependency detected in prefab hierarchy! GUID: {}",
                         prefabId.m_guid.to_hex());
            return true;
        }
    }

    // Add this prefab to the visited set
    visitedPrefabs.push_back(prefabId);

    // Load the prefab data (try cache first)
    std::string guidStr = prefabId.m_guid.to_hex();
    PrefabData* prefabData = nullptr;

    // Check if cached
    auto cacheIt = s_PrefabCache.find(guidStr);
    if (cacheIt != s_PrefabCache.end())
    {
        prefabData = &cacheIt->second;
    }
    else
    {
        spdlog::warn("Prefab not in cache for circular dependency check: {}", guidStr);
        // Could not find prefab - can't check dependencies
        // This is not necessarily a circular dependency, just missing data
        visitedPrefabs.pop_back(); // Remove from visited since we can't check it
        return false;
    }

    // Collect all nested prefab references
    std::vector<rp::BasicIndexedGuid> nestedPrefabs;
    CollectNestedPrefabReferences(prefabData->root, nestedPrefabs);

    // Recursively check each nested prefab
    for (const auto& nestedPrefabId : nestedPrefabs)
    {
        if (DetectCircularDependency(nestedPrefabId, visitedPrefabs))
        {
            spdlog::error("Circular dependency chain includes: {}", prefabId.m_guid.to_hex());
            return true; // Circular dependency found in child
        }
    }

    // Remove this prefab from visited (backtrack)
    visitedPrefabs.pop_back();

    return false; // No circular dependency found
}

bool PrefabSystem::DetectCircularDependency(const rp::BasicIndexedGuid& prefabId)
{
    std::vector<rp::BasicIndexedGuid> visitedPrefabs;
    return DetectCircularDependency(prefabId, visitedPrefabs);
}

std::vector<rp::BasicIndexedGuid> PrefabSystem::GetNestedPrefabDependencies(const rp::BasicIndexedGuid& prefabId)
{
    std::vector<rp::BasicIndexedGuid> dependencies;

    // Load the prefab data (try cache first)
    std::string guidStr = prefabId.m_guid.to_hex();
    auto cacheIt = s_PrefabCache.find(guidStr);

    if (cacheIt == s_PrefabCache.end())
    {
        spdlog::warn("Prefab not in cache for dependency collection: {}", guidStr);
        return dependencies; // Empty
    }

    PrefabData& prefabData = cacheIt->second;

    // Collect nested prefabs (recursively)
    CollectNestedPrefabReferences(prefabData.root, dependencies);

    return dependencies;
}

void PrefabSystem::CollectNestedPrefabReferences(const PrefabEntity& prefabEntity,
                                                 std::vector<rp::BasicIndexedGuid>& outNestedPrefabs)
{
    // Add all nested prefab references at this level
    for (const auto& nestedPrefab : prefabEntity.nestedPrefabs)
    {
        outNestedPrefabs.push_back(nestedPrefab.prefabGuid);

        // Recursively collect from the nested prefab's children
        // (if we have it cached, we can traverse deeper)
        std::string guidStr = nestedPrefab.prefabGuid.m_guid.to_hex();
        auto cacheIt = s_PrefabCache.find(guidStr);
        if (cacheIt != s_PrefabCache.end())
        {
            CollectNestedPrefabReferences(cacheIt->second.root, outNestedPrefabs);
        }
    }

    // Recursively traverse regular children (not nested prefabs)
    for (const auto& child : prefabEntity.children)
    {
        CollectNestedPrefabReferences(child, outNestedPrefabs);
    }
}
