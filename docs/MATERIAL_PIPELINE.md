# Material Pipeline Architecture

## Table of Contents
- [Overview](#overview)
- [Material vs Other Assets](#material-vs-other-assets)
- [Material Descriptor Format](#material-descriptor-format)
- [Material Sources](#material-sources)
- [Pipeline Flow](#pipeline-flow)
- [GUID Resolution](#guid-resolution)
- [Runtime Loading](#runtime-loading)
- [Comparison with Unity](#comparison-with-unity)
- [FAQ](#faq)

---

## Overview

The material pipeline processes material descriptors (`.desc` files) and compiles them into binary `.material` files for runtime use. Unlike textures and meshes which have external source files, **material descriptors ARE the source data**.

### Key Principles

1. **Materials are composites** - They reference other assets (shaders, textures) by GUID
2. **Descriptors are source** - The `.desc` file contains all material data
3. **Binary compilation** - Materials are compiled to `.material` binaries for fast runtime loading
4. **GUID-based references** - All texture/shader references use GUIDs, not file paths

---

## Material vs Other Assets

### Textures & Meshes (External Source Files)

```
Source File (.png, .fbx)
    ↓
Descriptor (.desc) - import settings
    ↓
Pipeline - compile/optimize
    ↓
Binary (.texture, .mesh)
```

**Characteristics:**
- External source file (artist-created)
- Descriptor contains import settings
- Heavy processing (compression, optimization)
- Self-contained data (no references)

---

### Materials (Descriptor IS Source)

```
Descriptor (.desc) - material definition
    ↓
Pipeline - serialize
    ↓
Binary (.material)
```

**Characteristics:**
- No external source file (descriptor is the source)
- Contains references to other assets (shaders, textures)
- Lightweight processing (serialization only)
- Composite asset (references resolved at runtime)

---

## Material Descriptor Format

### Structure

```yaml
# MyMaterial.desc
base:
  m_guid: "a1b2c3d4e5f6..."           # Unique material GUID
  m_name: "MyMaterial"                # Material name
  m_importer: ".mtl"                  # Source type (if from .mtl)
  m_importer_type: 12345              # Type hash

material:                             # MaterialResourceData
  vert_name: "main_pbr.vert"          # Vertex shader
  frag_name: "main_pbr.frag"          # Fragment shader
  material_name: "MyMaterial"

  # PBR Properties
  albedo: [0.8, 0.7, 0.6]             # RGB color
  metallic: 0.7                       # 0.0 - 1.0
  roughness: 0.3                      # 0.0 - 1.0

  # Custom Properties
  float_properties:
    emissiveStrength: 1.5
    alphaCutoff: 0.5

  vec3_properties:
    emissiveColor: [1.0, 0.8, 0.3]

  vec4_properties:
    tintColor: [1.0, 1.0, 1.0, 1.0]

  # Texture References (by GUID)
  texture_properties:
    u_AlbedoMap: "def456..."          # Albedo texture GUID
    u_NormalMap: "789xyz..."          # Normal map GUID
    u_MetallicRoughnessMap: "abc123..." # Metallic/roughness GUID
```

### Field Descriptions

| Field | Type | Purpose |
|-------|------|---------|
| `base.m_guid` | GUID | Unique identifier for this material |
| `base.m_name` | string | Human-readable name |
| `vert_name` | string | Vertex shader filename |
| `frag_name` | string | Fragment shader filename |
| `albedo` | vec3 | Base color (RGB) |
| `metallic` | float | Metallic factor (0=dielectric, 1=metal) |
| `roughness` | float | Surface roughness (0=smooth, 1=rough) |
| `float_properties` | map | Custom float properties |
| `vec3_properties` | map | Custom vec3 properties |
| `vec4_properties` | map | Custom vec4 properties |
| `texture_properties` | map<string, GUID> | Texture slot → texture GUID mappings |

---

## Material Sources

Materials can enter the pipeline from two sources:

### 1. Manual Creation (Editor/Tools)

User creates a material descriptor directly:

```yaml
# Created in editor or text editor
base:
  m_guid: "generate_new_guid"
  m_name: "WoodMaterial"

material:
  albedo: [0.6, 0.4, 0.2]
  metallic: 0.0
  roughness: 0.8
  texture_properties:
    u_AlbedoMap: "wood_albedo_guid"
    u_NormalMap: "wood_normal_guid"
```

**Workflow:**
1. Create `.desc` file manually
2. AssetManager detects new file
3. Pipeline compiles to `.material`
4. Ready for use

---

### 2. Extracted from 3D Models (.obj + .mtl)

Materials are extracted during `.obj` import:

```cpp
// Import character.obj (automatically loads character.mtl)
ImportOBJWithMaterials("character.obj") {
    // Assimp parses both .obj and .mtl
    aiScene* scene = importer.ReadFile("character.obj");

    // Extract each material from .mtl
    for (aiMaterial* aiMat : scene->mMaterials) {
        // Convert Assimp material → MaterialResourceData
        MaterialResourceData data = ConvertAssimpMaterial(aiMat);

        // Generate GUID
        rp::Guid guid = rp::Guid::generate();

        // Create descriptor
        MaterialDescriptor desc;
        desc.base.m_guid = guid;
        desc.base.m_name = aiMat->GetName();
        desc.material = data;

        // Save as .desc file
        SaveYAML(desc, "Body_Material.desc");
    }
}
```

**Workflow:**
1. User imports `character.obj`
2. Assimp reads companion `character.mtl`
3. Materials extracted from `.mtl`
4. `.desc` files created for each material
5. Pipeline compiles to `.material` binaries

**Result:**
```
Assets/Models/
├─ character.obj           (source - mesh)
├─ character.mtl           (source - materials, referenced by .desc)
├─ Body_Material.desc      (generated material descriptor)
├─ Hair_Material.desc      (generated material descriptor)
└─ character.desc          (mesh descriptor)

.imports/
├─ abc123.material         (Body material binary)
├─ def456.material         (Hair material binary)
└─ xyz789.mesh            (Mesh binary with material refs)
```

---

## Pipeline Flow

### Complete Material Pipeline

```
┌─────────────────────────────────────────────┐
│            MATERIAL SOURCES                 │
├─────────────────────────────────────────────┤
│                                             │
│  Path A: Manual Creation                    │
│  └─ Editor creates MyMaterial.desc          │
│                                             │
│  Path B: Extracted from .mtl                │
│  └─ .obj import → Body_Material.desc        │
│                                             │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│         MATERIAL PIPELINE INPUT             │
│                                             │
│         *.desc files (YAML)                 │
│                                             │
│  MaterialDescriptor {                       │
│    base: { guid, name, importer }           │
│    material: MaterialResourceData           │
│  }                                          │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│    ASSET MANAGER: File Detection            │
│                                             │
│  FileIndexingWorkerLoop() {                 │
│    Scan Assets/ directory                   │
│    if (file == "*.desc") {                  │
│      m_FileList.add(desc_path)              │
│    }                                        │
│  }                                          │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│    MATERIAL PIPELINE: Import                │
│                                             │
│  ImportAsset(desc_path) {                   │
│    1. Load YAML descriptor                  │
│    2. Extract MaterialResourceData          │
│    3. Validate texture GUIDs                │
│    4. Serialize to binary                   │
│       └─ .imports/abc123.material           │
│    5. Register in ResourceSystem            │
│       └─ GUID → FileEntry mapping           │
│  }                                          │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│         MATERIAL PIPELINE OUTPUT            │
│                                             │
│         .material binaries                  │
│                                             │
│  Binary serialized MaterialResourceData:    │
│  - Shader names                             │
│  - PBR properties                           │
│  - Texture GUID references                  │
│  - Custom properties                        │
└─────────────────────────────────────────────┘
```

### Step-by-Step Process

#### Step 1: Descriptor Creation/Detection

```cpp
// AssetManager.cpp
void FileIndexingWorkerLoop() {
    for (auto& entry : filesystem::recursive_directory_iterator(m_RootPath)) {
        string ext = getFileExtension(entry.path());

        // Skip output/intermediate files
        if (ext == ".texture" || ext == ".mesh" || ext == ".desc" || ext == ".mtl") {
            continue;
        }

        // Check if descriptor exists
        string descPath = entry.path().replace_extension(".desc");
        if (!filesystem::exists(descPath)) {
            CreateDefaultDescriptor(entry.path());
        }

        m_FileList.emplace(directory, descPath);
    }
}
```

#### Step 2: Import Descriptor

```cpp
void ImportAsset(const string& descPath) {
    // 1. Get importer type from descriptor
    uint64_t importerType = GetDescriptorImporterType(descPath);

    // 2. Get GUID from descriptor
    BasicIndexedGuid bguid = GetDescriptorGuid(descPath);

    // 3. Invoke registered importer
    ResourceTypeImporterRegistry::Import(importerType, descPath);

    // 4. Register in ResourceSystem
    FileEntry entry;
    entry.m_Guid = bguid.m_guid;
    entry.m_Path = ".imports/" + bguid.m_guid.to_hex() + ".material";
    entry.m_Size = filesystem::file_size(entry.m_Path);

    ResourceSystem::Instance().m_FileEntries[entry.m_Guid] = entry;

    // 5. Add to AssetManager mappings
    m_AssetNameGuid[GetDescriptorName(descPath)] = bguid;
    m_AssetReverse[bguid] = GetDescriptorName(descPath);
}
```

#### Step 3: Material-Specific Import

```cpp
// Registered via RegisterResourceTypeImporter macro
void ImportMaterial(const MaterialDescriptor& desc) {
    // Material data is embedded in descriptor
    const MaterialResourceData& data = desc.material;

    // Serialize to binary
    string outputPath = ".imports/" + desc.base.m_guid.to_hex() + ".material";
    binary_serializer::serialize(data, outputPath);
}
```

---

## GUID Resolution

Materials reference textures and shaders by GUID. These references must be resolved at runtime.

### Texture Resolution

```cpp
// From Render.cpp material loading
REGISTER_RESOURCE_TYPE_SHARED_PTR(Material,
    [](const char* data) -> shared_ptr<Material> {
        // Deserialize binary
        MaterialResourceData matData = binary_serializer::deserialize<MaterialResourceData>(
            reinterpret_cast<const byte*>(data)
        );

        // Load shader
        shared_ptr<Shader> shader = LoadShader(
            matData.material_name + "_shader",
            matData.vert_name,
            matData.frag_name
        );

        // Create material
        shared_ptr<Material> mat = make_shared<Material>(shader, matData.material_name);

        // Set properties
        mat->SetAlbedoColor(matData.albedo);
        mat->SetRoughnessValue(matData.roughness);
        mat->SetMetallicValue(matData.metallic);

        // Resolve texture GUIDs
        for (const auto& [texName, texGuid] : matData.texture_properties) {
            if (texGuid == rp::null_guid) continue;

            // Find texture in ResourceRegistry
            auto& registry = ResourceRegistry::Instance();
            Handle texHandle = registry.Find<shared_ptr<Texture>>(texGuid);

            if (texHandle) {
                auto* pool = registry.Pool<shared_ptr<Texture>>();
                auto* texPtr = pool->Ptr(texHandle);
                if (texPtr) {
                    mat->SetTexture(texName, *texPtr);  // Assign texture
                }
            } else {
                spdlog::warn("Material '{}': Texture '{}' (GUID {}) not found",
                    matData.material_name, texName, texGuid.to_hex());
            }
        }

        return mat;
    },
    [](shared_ptr<Material>&) {});
```

### GUID Resolution Flow

```
Material Load Request (by GUID)
    ↓
ResourceRegistry::Get<Material>(materialGuid)
    ↓
Find FileEntry by GUID
    ↓
Load .material binary
    ↓
Deserialize MaterialResourceData
    ↓
For each texture in texture_properties:
    ↓
    ResourceRegistry::Get<Texture>(textureGuid)
        ↓
        Find FileEntry by texture GUID
        ↓
        Load .texture binary (if not loaded)
        ↓
        Return Texture*
    ↓
    mat->SetTexture(name, texture)
    ↓
Return fully-loaded Material
```

---

## Runtime Loading

### Loading by GUID

```cpp
// Typical usage in rendering code
void RenderSystem::Update(ecs::world& world) {
    for (auto entity : world.view<MeshRendererComponent>()) {
        auto& meshComp = entity.get<MeshRendererComponent>();

        // Load material by GUID
        rp::Guid materialGuid = meshComp.m_MaterialGuid.m_guid;

        shared_ptr<Material> mat = LoadMaterialResource(materialGuid);

        // Material is now fully loaded with all textures resolved
        renderer.DrawMesh(mesh, mat);
    }
}
```

### Caching and Performance

**First Load:**
1. Find FileEntry by GUID → `.imports/abc123.material`
2. Read binary file
3. Deserialize MaterialResourceData
4. Resolve all texture GUIDs (may trigger texture loads)
5. Create Material object
6. Register in ResourceRegistry (cached)

**Subsequent Loads:**
1. Check ResourceRegistry cache
2. Return cached Material immediately (no file I/O)

---

## Comparison with Unity

### Asset Types Comparison

| Asset Type | Unity | Your Engine |
|------------|-------|-------------|
| **Textures** | `.png` → Library/ cache | `.png` + `.desc` → `.texture` binary |
| **Meshes** | `.fbx` → Library/ cache | `.fbx` + `.desc` → `.mesh` binary |
| **Materials** | `.mat` (YAML, direct) | `.desc` (YAML) → `.material` (binary) |

### Material Workflow

#### Unity:

```
Create Material
    ↓
MyMaterial.mat (YAML)
    ↓
Runtime: Parse YAML directly
    ↓
Resolve shader/texture GUIDs
    ↓
Use material
```

**Pros:**
- ✅ Instant iteration (edit .mat, see changes immediately)
- ✅ Human-readable source
- ✅ No import step

**Cons:**
- 🟡 Slower load times (YAML parsing)
- 🟡 Larger file sizes

---

#### Your Engine:

```
Create Material
    ↓
MyMaterial.desc (YAML)
    ↓
Import: Compile to binary
    ↓
.material binary
    ↓
Runtime: Load binary
    ↓
Resolve texture GUIDs
    ↓
Use material
```

**Pros:**
- ✅ Faster load times (binary deserialization)
- ✅ Smaller file sizes
- ✅ Consistent pipeline (all assets use descriptors)

**Cons:**
- 🟡 Extra import step (but cached)
- 🟡 Need to reimport after editing

### Reference Architecture

Both systems use the same **GUID-based reference** architecture:

```
Material
  ├─ Shader (by name or GUID)
  ├─ Albedo: vec3
  ├─ Metallic: float
  └─ Textures (by GUID)
      ├─ u_AlbedoMap: GUID → Texture*
      ├─ u_NormalMap: GUID → Texture*
      └─ u_RoughnessMap: GUID → Texture*
```

**Unity:** References stored in YAML, resolved at runtime
**Your Engine:** References stored in binary, resolved at runtime

---

## FAQ

### Q: Should I process .mtl files in the material pipeline?

**No.** `.mtl` files are:
- Source data for `.obj` models (Wavefront format)
- Automatically parsed by Assimp during `.obj` import
- Should be skipped by AssetManager
- Used to **generate** `.desc` files, not processed directly

### Q: Where do materials from .obj imports go?

When you import `character.obj`:
1. Assimp reads companion `character.mtl`
2. Materials are extracted and converted to your format
3. `.desc` files are created for each material
4. These `.desc` files enter the material pipeline
5. Compiled to `.material` binaries

### Q: Can I edit materials after importing?

**Yes!** Edit the `.desc` file:
1. Open `MyMaterial.desc` in text editor
2. Modify properties (colors, textures, etc.)
3. Save file
4. AssetManager detects change
5. Reimports and recompiles to `.material`
6. Changes appear in engine (may need asset refresh)

### Q: How do I reference textures in materials?

Use texture GUIDs in the `texture_properties` map:

```yaml
texture_properties:
  u_AlbedoMap: "abc123def456..."  # Texture GUID
  u_NormalMap: "789xyz..."
```

The GUID is found in the texture's `.desc` file:

```yaml
# AlbedoTexture.desc
base:
  m_guid: "abc123def456..."  # ← This GUID
```

### Q: What happens if a texture GUID is invalid?

At runtime:
1. Material tries to resolve texture GUID
2. ResourceRegistry lookup fails
3. Warning is logged
4. Material uses fallback/default texture (or null)
5. Rendering continues (won't crash)

### Q: How do I create a new material from scratch?

**Option 1: Manual .desc creation**
```yaml
base:
  m_guid: "generate_new_guid_here"
  m_name: "MyNewMaterial"
  m_importer_type: 12345

material:
  vert_name: "main_pbr.vert"
  frag_name: "main_pbr.frag"
  material_name: "MyNewMaterial"
  albedo: [0.8, 0.7, 0.6]
  metallic: 0.5
  roughness: 0.5
```

**Option 2: Editor tool (future)**
- Material editor UI
- Property inspector
- Texture drag-and-drop
- Auto-generates `.desc` file

### Q: Can materials share textures?

**Yes!** Multiple materials can reference the same texture GUID:

```yaml
# Material1.desc
texture_properties:
  u_AlbedoMap: "shared_texture_guid"

# Material2.desc
texture_properties:
  u_AlbedoMap: "shared_texture_guid"  # Same GUID
```

The texture is loaded once and shared by both materials (reference counted).

### Q: What's the difference between Unity's Library/ and your .imports/?

Both serve the same purpose:

| Aspect | Unity Library/ | Your .imports/ |
|--------|---------------|----------------|
| **Purpose** | Cached compiled assets | Compiled binary assets |
| **Textures** | Compressed textures | `.texture` binaries |
| **Meshes** | Optimized meshes | `.mesh` binaries |
| **Materials** | ❌ NOT cached (direct YAML) | `.material` binaries |
| **Version Control** | ❌ Don't commit | ❌ Don't commit (add to .gitignore) |

---

## Best Practices

### 1. Organize Materials by Usage

```
Assets/Materials/
├─ Characters/
│   ├─ Skin_Material.desc
│   └─ Hair_Material.desc
├─ Environment/
│   ├─ Grass_Material.desc
│   └─ Stone_Material.desc
└─ Props/
    └─ Metal_Material.desc
```

### 2. Use Descriptive Texture Slot Names

```yaml
texture_properties:
  u_AlbedoMap: "..."        # ✅ Clear
  u_NormalMap: "..."        # ✅ Clear
  u_ORMMap: "..."           # ✅ Clear (Occlusion/Roughness/Metallic)

  # Avoid:
  texture0: "..."           # ❌ Unclear
  tex: "..."                # ❌ Ambiguous
```

### 3. Keep .mtl Files for Re-import

Don't delete `.mtl` files after importing:
- Source of truth for 3D model materials
- Needed for re-importing if source changes
- Can be re-processed with different settings

### 4. Use Consistent Shader Paths

```yaml
# Recommended:
vert_name: "main_pbr.vert"
frag_name: "main_pbr.frag"

# Avoid absolute paths:
vert_name: "D:/assets/shaders/pbr.vert"  # ❌ Not portable
```

### 5. Validate Texture GUIDs

Before assigning texture GUID, verify it exists:
```cpp
if (ResourceSystem::Instance().FindFile(texGuid)) {
    mat.texture_properties["u_AlbedoMap"] = texGuid;  // ✅ Valid
} else {
    spdlog::warn("Invalid texture GUID: {}", texGuid.to_hex());
}
```

---

## Troubleshooting

### Materials don't load textures

**Symptom:** Material loads but appears untextured

**Causes:**
1. Invalid texture GUID in descriptor
2. Texture not imported yet
3. Texture binary missing from `.imports/`

**Solution:**
```cpp
// Check ResourceSystem logs
spdlog::warn("Material: Texture GUID {} not found");

// Verify texture exists:
ls .imports/abc123.texture  # Should exist
```

### Materials from .obj import are lost

**Symptom:** `.obj` imports but no materials created

**Causes:**
1. `.mtl` file missing
2. `.mtl` parsing failed
3. Material extraction not implemented

**Solution:**
Ensure `.obj` and `.mtl` are in same directory:
```
character.obj
character.mtl  ← Must be present
```

### Descriptor changes don't appear in engine

**Symptom:** Edit `.desc` file but material unchanged

**Causes:**
1. Asset manager not rescanning
2. Binary not recompiled
3. Material cached in ResourceRegistry

**Solution:**
1. Force asset rescan
2. Delete old `.material` binary
3. Clear ResourceRegistry cache

---

## See Also

- `TEXTURE_PIPELINE.md` - Texture import and processing
- `MESH_PIPELINE.md` - Mesh import and processing
- `RESOURCE_SYSTEM.md` - GUID-based resource loading
- `ASSET_MANAGER.md` - File scanning and import orchestration
