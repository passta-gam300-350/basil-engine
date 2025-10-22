# Material Property Serialization Implementation Plan
## Using Both Reflection System AND Resource System

**Date**: 2025-10-22
**Goal**: Implement generic material property serialization using EnTT reflection + Resource system

---

## System Integration Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    MATERIAL ASSET PIPELINE                       │
└─────────────────────────────────────────────────────────────────┘

1. EDITOR (Creation/Editing)
   ├─ Use Reflection for ImGui Inspector
   ├─ MaterialAssetData ←→ YAML via Ser

ializeType()
   └─ Save as: material.yaml (human-readable)

2. ASSET IMPORT (Cooking)
   ├─ Load: material.yaml
   ├─ Deserialize via reflection → MaterialAssetData
   ├─ Binary serialize: MaterialAssetData::DumpToMemory()
   └─ Save as: material.bin (packed asset)

3. RUNTIME LOADING (Resource System)
   ├─ Load: material.bin via ResourceRegistry
   ├─ Binary deserialize: load_native_material_from_memory()
   ├─ Resolve dependencies (shader GUID, texture GUIDs)
   ├─ Create Material instance
   └─ Apply all properties from MaterialAssetData

┌─────────────────────────────────────────────────────────────────┐
│                         KEY INSIGHT                              │
│                                                                  │
│  Reflection System: Editor/Tools (YAML ←→ Struct)              │
│  Resource System: Runtime Loading (Binary ←→ Struct)            │
│  Both operate on the SAME MaterialAssetData structure           │
└─────────────────────────────────────────────────────────────────┘
```

---

## Phase 1: Material Property Structure

### 1.1 MaterialProperty Struct (Reflection-Friendly)

```cpp
// File: lib/resource/core/include/native/material.h

namespace Resource {
    /**
     * Property type enumeration
     * These map 1:1 with shader uniform types
     */
    enum class MaterialPropertyType : std::uint8_t {
        FLOAT = 0,
        VEC2 = 1,
        VEC3 = 2,
        VEC4 = 3,
        MAT3 = 4,
        MAT4 = 5,
        TEXTURE = 6,  // Stored as Resource::Guid
        INT = 7,
        BOOL = 8
    };

    /**
     * Single material property with union-based storage
     *
     * Reflection-friendly: All members are directly accessible
     * Binary-friendly: Fixed-size union for efficient serialization
     */
    struct MaterialProperty {
        std::string name;                          // Property name (e.g., "u_Roughness")
        MaterialPropertyType type = MaterialPropertyType::FLOAT;

        // Value storage (only one is active based on 'type')
        float floatValue = 0.0f;
        int intValue = 0;
        bool boolValue = false;
        glm::vec2 vec2Value = glm::vec2(0.0f);
        glm::vec3 vec3Value = glm::vec3(0.0f);
        glm::vec4 vec4Value = glm::vec4(0.0f);
        glm::mat3 mat3Value = glm::mat3(1.0f);
        glm::mat4 mat4Value = glm::mat4(1.0f);
        Resource::Guid textureGuid = {};           // For texture references

        // Helper constructors
        static MaterialProperty Float(const std::string& name, float value);
        static MaterialProperty Vec3(const std::string& name, const glm::vec3& value);
        static MaterialProperty Texture(const std::string& name, const Resource::Guid& guid);
    };
}
```

**Why this design?**
- ✅ No C++ unions (reflection-unfriendly) - separate fields instead
- ✅ EnTT reflection can iterate all fields automatically
- ✅ Binary serialization knows exact size of each type
- ✅ Type safety via enum discriminator

### 1.2 MaterialAssetData V2

```cpp
// File: lib/resource/core/include/native/material.h

namespace Resource {
    /**
     * Material asset data with generic property storage
     *
     * V2 Format Changes:
     * - Generic property vector (replaces 3 hardcoded PBR properties)
     * - Version field for backward compatibility
     * - Texture GUID support
     *
     * Integration:
     * - Binary serialization: DumpToMemory() / load_native_material_from_memory()
     * - YAML serialization: Via EnTT reflection (SerializeType/DeserializeType)
     * - Resource loading: Via ResourceRegistry with loader lambda
     */
    struct MaterialAssetData {
        static constexpr uint64_t MATERIAL_MAGIC_VALUE{ iso8859ToBinary("E.MAT") };
        static constexpr uint32_t VERSION_1 = 1;  // Legacy format (3 hardcoded properties)
        static constexpr uint32_t VERSION_2 = 2;  // Current format (generic properties)

        std::uint32_t version = VERSION_2;        // File format version
        Resource::Guid shader_guid;                // Shader asset reference
        std::string m_Name;                        // Material name

        // Generic property storage (NEW in V2)
        std::vector<MaterialProperty> properties;

        // Binary serialization (Resource System)
        MaterialAssetData& operator>>(std::ostream& outp);
        MaterialAssetData const& operator>>(std::ostream& outp) const;
        std::uint64_t DumpToMemory(char* buff, std::uint64_t buffer_size) const;

        // Helper methods
        void AddProperty(const MaterialProperty& prop);
        MaterialProperty* FindProperty(const std::string& name);
        const MaterialProperty* FindProperty(const std::string& name) const;

        // Migration: Convert V1 (old format) to V2 (new format)
        static MaterialAssetData MigrateFromV1(
            const glm::vec3& albedo,
            float metallic,
            float roughness);
    };
}
```

---

## Phase 2: Reflection System Integration

### 2.1 Register MaterialPropertyType Enum

```cpp
// File: engine/src/Ecs/internal/reflection.cpp

void ReflectionRegistry::SetupNativeTypes() {
    // ... existing native type registrations ...

    // Register MaterialPropertyType enum
    entt::meta<Resource::MaterialPropertyType>()
        .type("MaterialPropertyType"_hs)
        .data<Resource::MaterialPropertyType::FLOAT>("FLOAT"_hs)
        .data<Resource::MaterialPropertyType::VEC2>("VEC2"_hs)
        .data<Resource::MaterialPropertyType::VEC3>("VEC3"_hs)
        .data<Resource::MaterialPropertyType::VEC4>("VEC4"_hs)
        .data<Resource::MaterialPropertyType::MAT3>("MAT3"_hs)
        .data<Resource::MaterialPropertyType::MAT4>("MAT4"_hs)
        .data<Resource::MaterialPropertyType::TEXTURE>("TEXTURE"_hs)
        .data<Resource::MaterialPropertyType::INT>("INT"_hs)
        .data<Resource::MaterialPropertyType::BOOL>("BOOL"_hs);
}
```

### 2.2 Register MaterialProperty Struct

```cpp
// File: engine/src/Ecs/internal/reflection.cpp

void ReflectionRegistry::SetupEngineTypes() {
    // ... existing engine type registrations ...

    // Register MaterialProperty
    auto prop_type_id = entt::type_hash<Resource::MaterialProperty>::value();
    auto& prop_field_names = ReflectionRegistry::RegisterType(
        prop_type_id, "MaterialProperty");

    entt::meta<Resource::MaterialProperty>()
        .data<&Resource::MaterialProperty::name>("name"_hs)
        .data<&Resource::MaterialProperty::type>("type"_hs)
        .data<&Resource::MaterialProperty::floatValue>("floatValue"_hs)
        .data<&Resource::MaterialProperty::intValue>("intValue"_hs)
        .data<&Resource::MaterialProperty::boolValue>("boolValue"_hs)
        .data<&Resource::MaterialProperty::vec2Value>("vec2Value"_hs)
        .data<&Resource::MaterialProperty::vec3Value>("vec3Value"_hs)
        .data<&Resource::MaterialProperty::vec4Value>("vec4Value"_hs)
        .data<&Resource::MaterialProperty::mat3Value>("mat3Value"_hs)
        .data<&Resource::MaterialProperty::mat4Value>("mat4Value"_hs)
        .data<&Resource::MaterialProperty::textureGuid>("textureGuid"_hs);

    // Store field name mappings for serialization
    prop_field_names[entt::hashed_string("name")] = "name";
    prop_field_names[entt::hashed_string("type")] = "type";
    prop_field_names[entt::hashed_string("floatValue")] = "floatValue";
    // ... etc for all fields
}
```

### 2.3 Register MaterialAssetData

```cpp
// File: engine/src/Ecs/internal/reflection.cpp

void ReflectionRegistry::SetupEngineTypes() {
    // ... existing registrations ...

    // Register MaterialAssetData
    auto mat_type_id = entt::type_hash<Resource::MaterialAssetData>::value();
    auto& mat_field_names = ReflectionRegistry::RegisterType(
        mat_type_id, "MaterialAssetData");

    entt::meta<Resource::MaterialAssetData>()
        .data<&Resource::MaterialAssetData::version>("version"_hs)
        .data<&Resource::MaterialAssetData::shader_guid>("shader_guid"_hs)
        .data<&Resource::MaterialAssetData::m_Name>("m_Name"_hs)
        .data<&Resource::MaterialAssetData::properties>("properties"_hs);

    mat_field_names[entt::hashed_string("version")] = "version";
    mat_field_names[entt::hashed_string("shader_guid")] = "shader_guid";
    mat_field_names[entt::hashed_string("m_Name")] = "m_Name";
    mat_field_names[entt::hashed_string("properties")] = "properties";
}
```

**Result:** MaterialAssetData can now be serialized via reflection:
```cpp
// YAML serialization (for editor)
Resource::MaterialAssetData matData;
YAML::Node node;
auto meta_any = entt::meta_any(std::ref(matData));
SerializeType(meta_any, node);
// node now contains all properties in YAML format

// YAML deserialization
DeserializeType(node, meta_any);
// matData populated from YAML
```

---

## Phase 3: Binary Serialization (Resource System)

### 3.1 Binary Format V2

```
┌────────────────────────────────────────────────────────────┐
│                   MATERIAL BINARY FORMAT V2                 │
└────────────────────────────────────────────────────────────┘

[MAGIC_VALUE]        uint64_t (iso8859ToBinary("E.MAT"))
[VERSION]            uint32_t (2 for current version)
[SHADER_GUID]        Resource::Guid (16 bytes)
[NAME_LENGTH]        uint64_t
[NAME_DATA]          char[NAME_LENGTH]
[PROPERTY_COUNT]     uint64_t

For each property:
  [PROP_NAME_LENGTH] uint64_t
  [PROP_NAME_DATA]   char[PROP_NAME_LENGTH]
  [PROP_TYPE]        uint8_t (MaterialPropertyType enum)
  [PROP_VALUE]       varies by type:
                     - FLOAT: 4 bytes
                     - VEC3: 12 bytes
                     - VEC4: 16 bytes
                     - MAT4: 64 bytes
                     - TEXTURE: 16 bytes (Guid)
                     etc.
```

### 3.2 Serializer Implementation

```cpp
// File: lib/resource/core/src/saver.cpp

std::uint64_t MaterialAssetData::DumpToMemory(char* buff, std::uint64_t size) const {
    // Calculate total size needed
    std::uint64_t dumpsize =
        sizeof(MATERIAL_MAGIC_VALUE) +
        sizeof(version) +
        sizeof(shader_guid) +
        sizeof(std::uint64_t) +           // name length
        m_Name.size() +
        sizeof(std::uint64_t);            // property count

    // Add size for each property
    for (const auto& prop : properties) {
        dumpsize += sizeof(std::uint64_t);  // name length
        dumpsize += prop.name.size();
        dumpsize += sizeof(MaterialPropertyType);  // type

        // Add value size based on type
        switch (prop.type) {
            case MaterialPropertyType::FLOAT:
            case MaterialPropertyType::INT:
            case MaterialPropertyType::BOOL:
                dumpsize += 4;
                break;
            case MaterialPropertyType::VEC2:
                dumpsize += sizeof(glm::vec2);
                break;
            case MaterialPropertyType::VEC3:
                dumpsize += sizeof(glm::vec3);
                break;
            case MaterialPropertyType::VEC4:
                dumpsize += sizeof(glm::vec4);
                break;
            case MaterialPropertyType::MAT3:
                dumpsize += sizeof(glm::mat3);
                break;
            case MaterialPropertyType::MAT4:
                dumpsize += sizeof(glm::mat4);
                break;
            case MaterialPropertyType::TEXTURE:
                dumpsize += sizeof(Resource::Guid);
                break;
        }
    }

    assert(size >= dumpsize && "Buffer too small!");

    // Write header
    MemWrite(buff, &MATERIAL_MAGIC_VALUE);
    MemWrite(buff, version);
    MemWrite(buff, shader_guid);
    MemWrite(buff, m_Name.size());
    MemWrite(buff, reinterpret_cast<const char*>(m_Name.data()), m_Name.size());

    // Write property count
    MemWrite(buff, static_cast<std::uint64_t>(properties.size()));

    // Write each property
    for (const auto& prop : properties) {
        // Property name
        MemWrite(buff, prop.name.size());
        MemWrite(buff, reinterpret_cast<const char*>(prop.name.data()), prop.name.size());

        // Property type
        MemWrite(buff, static_cast<std::uint8_t>(prop.type));

        // Property value (based on type)
        switch (prop.type) {
            case MaterialPropertyType::FLOAT:
                MemWrite(buff, reinterpret_cast<const char*>(&prop.floatValue), sizeof(float));
                break;
            case MaterialPropertyType::VEC3:
                MemWrite(buff, reinterpret_cast<const char*>(&prop.vec3Value), sizeof(glm::vec3));
                break;
            case MaterialPropertyType::VEC4:
                MemWrite(buff, reinterpret_cast<const char*>(&prop.vec4Value), sizeof(glm::vec4));
                break;
            case MaterialPropertyType::MAT4:
                MemWrite(buff, reinterpret_cast<const char*>(&prop.mat4Value), sizeof(glm::mat4));
                break;
            case MaterialPropertyType::TEXTURE:
                MemWrite(buff, reinterpret_cast<const char*>(&prop.textureGuid), sizeof(Resource::Guid));
                break;
            // ... handle other types
        }
    }

    return size - dumpsize;
}
```

### 3.3 Deserializer Implementation

```cpp
// File: lib/resource/core/src/loader.cpp

MaterialAssetData load_native_material_from_memory(const char* data) {
    const char* read_ptr{ data };

    // Read and verify magic value
    auto magic = MemRead<std::uint64_t>(read_ptr);
    assert(magic == MaterialAssetData::MATERIAL_MAGIC_VALUE && "Invalid material file!");

    // Read version
    std::uint32_t version = MemRead<std::uint32_t>(read_ptr);

    if (version == MaterialAssetData::VERSION_1) {
        // V1 format: Read old hardcoded properties and migrate
        Resource::Guid shader_guid = MemRead<Resource::Guid>(read_ptr);

        std::uint64_t strsize = MemRead<std::uint64_t>(read_ptr);
        std::string name;
        name.resize(strsize);
        MemRead(read_ptr, reinterpret_cast<char*>(name.data()), strsize);

        glm::vec3 albedo;
        float metallic, roughness;
        MemRead(read_ptr, reinterpret_cast<char*>(&albedo), sizeof(glm::vec3));
        MemRead(read_ptr, reinterpret_cast<char*>(&metallic), sizeof(float));
        MemRead(read_ptr, reinterpret_cast<char*>(&roughness), sizeof(float));

        // Migrate to V2
        auto result = MaterialAssetData::MigrateFromV1(albedo, metallic, roughness);
        result.shader_guid = shader_guid;
        result.m_Name = name;
        return result;
    }

    // V2 format: Read generic properties
    MaterialAssetData tmp{};
    tmp.version = version;

    // Read shader GUID
    tmp.shader_guid = MemRead<Resource::Guid>(read_ptr);

    // Read material name
    std::uint64_t nameSize = MemRead<std::uint64_t>(read_ptr);
    tmp.m_Name.resize(nameSize);
    MemRead(read_ptr, reinterpret_cast<char*>(tmp.m_Name.data()), nameSize);

    // Read property count
    std::uint64_t propCount = MemRead<std::uint64_t>(read_ptr);
    tmp.properties.reserve(propCount);

    // Read each property
    for (std::uint64_t i = 0; i < propCount; ++i) {
        MaterialProperty prop;

        // Property name
        std::uint64_t propNameSize = MemRead<std::uint64_t>(read_ptr);
        prop.name.resize(propNameSize);
        MemRead(read_ptr, reinterpret_cast<char*>(prop.name.data()), propNameSize);

        // Property type
        prop.type = static_cast<MaterialPropertyType>(MemRead<std::uint8_t>(read_ptr));

        // Property value (based on type)
        switch (prop.type) {
            case MaterialPropertyType::FLOAT:
                MemRead(read_ptr, reinterpret_cast<char*>(&prop.floatValue), sizeof(float));
                break;
            case MaterialPropertyType::VEC3:
                MemRead(read_ptr, reinterpret_cast<char*>(&prop.vec3Value), sizeof(glm::vec3));
                break;
            case MaterialPropertyType::VEC4:
                MemRead(read_ptr, reinterpret_cast<char*>(&prop.vec4Value), sizeof(glm::vec4));
                break;
            case MaterialPropertyType::MAT4:
                MemRead(read_ptr, reinterpret_cast<char*>(&prop.mat4Value), sizeof(glm::mat4));
                break;
            case MaterialPropertyType::TEXTURE:
                MemRead(read_ptr, reinterpret_cast<char*>(&prop.textureGuid), sizeof(Resource::Guid));
                break;
            // ... handle other types
        }

        tmp.properties.push_back(std::move(prop));
    }

    return tmp;
}
```

---

## Phase 4: Material Class Integration

### 4.1 Update Material to Use Generic Properties

```cpp
// File: engine/src/Render/Render.cpp

REGISTER_RESOURCE_TYPE_SHARED_PTR(Material,
    [](const char* data)->std::shared_ptr<Material> {
        // 1. Load MaterialAssetData from binary
        Resource::MaterialAssetData dat = Resource::load_native_material_from_memory(data);

        // 2. Resolve shader dependency
        Resource::ShaderAssetData* shdr_dat =
            ResourceRegistry::Instance().Get<Resource::ShaderAssetData>(dat.shader_guid);

        std::shared_ptr<Shader> shdr =
            Engine::GetRenderSystem().m_SceneRenderer->GetResourceManager()
                ->LoadShader(shdr_dat->m_Name, shdr_dat->m_VertPath, shdr_dat->m_FragPath);

        // 3. Create Material
        std::shared_ptr<Material> mat = std::make_shared<Material>(shdr, dat.m_Name);

        // 4. Apply ALL properties from asset data
        for (const auto& prop : dat.properties) {
            switch (prop.type) {
                case Resource::MaterialPropertyType::FLOAT:
                    mat->SetFloat(prop.name, prop.floatValue);
                    break;

                case Resource::MaterialPropertyType::VEC3:
                    mat->SetVec3(prop.name, prop.vec3Value);
                    break;

                case Resource::MaterialPropertyType::VEC4:
                    mat->SetVec4(prop.name, prop.vec4Value);
                    break;

                case Resource::MaterialPropertyType::MAT4:
                    mat->SetMat4(prop.name, prop.mat4Value);
                    break;

                case Resource::MaterialPropertyType::TEXTURE:
                    // Resolve texture GUID → Texture resource
                    if (prop.textureGuid) {
                        // TODO: Load texture from resource system
                        // auto* texData = ResourceRegistry::Instance()
                        //     .Get<Resource::TextureAssetData>(prop.textureGuid);
                        // mat->SetTexture(prop.name, texture);
                    }
                    break;

                // ... handle other types
            }
        }

        return mat;
    },
    [](std::shared_ptr<Material>&) {});  // Empty unloader
```

---

## Phase 5: Backward Compatibility

### 5.1 V1 to V2 Migration

```cpp
// File: lib/resource/core/src/material.cpp (new implementation file)

MaterialAssetData MaterialAssetData::MigrateFromV1(
    const glm::vec3& albedo,
    float metallic,
    float roughness)
{
    MaterialAssetData v2;
    v2.version = VERSION_2;

    // Convert hardcoded V1 properties to generic V2 properties
    v2.properties.push_back(MaterialProperty::Vec3("u_AlbedoColor", albedo));
    v2.properties.push_back(MaterialProperty::Float("u_MetallicValue", metallic));
    v2.properties.push_back(MaterialProperty::Float("u_RoughnessValue", roughness));

    return v2;
}
```

---

## Complete Workflow Example

### Editor Workflow (YAML + Reflection)

```cpp
// 1. CREATE MATERIAL IN EDITOR
Resource::MaterialAssetData matData;
matData.m_Name = "MyMaterial";
matData.shader_guid = GetShaderGuid("pbr_standard");
matData.properties.push_back(MaterialProperty::Vec3("u_AlbedoColor", glm::vec3(0.8f)));
matData.properties.push_back(MaterialProperty::Float("u_Roughness", 0.5f));
matData.properties.push_back(MaterialProperty::Texture("u_NormalMap", normalMapGuid));

// 2. SAVE TO YAML (via reflection)
YAML::Node node;
auto meta_any = entt::meta_any(std::ref(matData));
SerializeType(meta_any, node);
std::ofstream yaml_file("materials/MyMaterial.yaml");
yaml_file << node;

// 3. IMPORT TO BINARY (asset cooking)
YAML::Node loaded = YAML::LoadFile("materials/MyMaterial.yaml");
DeserializeType(loaded, meta_any);
char buffer[4096];
matData.DumpToMemory(buffer, sizeof(buffer));
std::ofstream bin_file("assets/materials.bin", std::ios::binary | std::ios::app);
bin_file.write(buffer, matData.CalculateSize());
```

### Runtime Workflow (Binary + Resource System)

```cpp
// 1. LOAD MATERIAL AT RUNTIME
Resource::Guid materialGuid = GetMaterialGuid("MyMaterial");
auto material = ResourceRegistry::Instance()
    .Get<std::shared_ptr<Material>>(materialGuid);

// Material is now loaded with all properties applied!

// 2. ASSIGN TO ENTITY
MeshRendererComponent renderer;
renderer.m_MaterialGuid = materialGuid;
renderer.hasAttachedMaterial = true;

// 3. RENDER
// RenderSystem automatically resolves GUID → Material → applies properties
```

---

## Benefits of Dual System Integration

| Aspect | Benefit |
|--------|---------|
| **Editor** | Use reflection for automatic UI generation |
| **Runtime** | Use binary for fast loading (no YAML parsing) |
| **Debugging** | YAML is human-readable and editable |
| **Performance** | Binary is compact and fast |
| **Extensibility** | Add properties without code changes (reflection auto-discovers) |
| **Type Safety** | Both systems validate types |
| **Versioning** | Binary format supports migration |

---

## Implementation Checklist

- [ ] Phase 1: Create MaterialProperty and MaterialAssetData V2 structures
- [ ] Phase 2: Register with reflection system
- [ ] Phase 3: Implement binary serialization/deserialization
- [ ] Phase 4: Update Material resource loader
- [ ] Phase 5: Add V1→V2 migration support
- [ ] Phase 6: Add helper methods (FindProperty, AddProperty)
- [ ] Phase 7: Write unit tests for serialization
- [ ] Phase 8: Update documentation

---

## Files to Modify

| File | Changes |
|------|---------|
| `lib/resource/core/include/native/material.h` | Add MaterialProperty, update MaterialAssetData |
| `lib/resource/core/src/loader.cpp` | Update load_native_material_from_memory() |
| `lib/resource/core/src/saver.cpp` | Update DumpToMemory() |
| `engine/src/Ecs/internal/reflection.cpp` | Register types with reflection |
| `engine/src/Render/Render.cpp` | Update Material loader lambda |
| `lib/Graphics/include/Resources/Material.h` | Add texture property support (if needed) |

---

This plan provides full integration with both the reflection system (for editor/tooling) and resource system (for runtime loading), giving you the best of both worlds!
