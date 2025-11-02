# Graphics Framework Editor Integration Roadmap

## Overview
This document outlines the roadmap for enhancing the graphics framework to support ECS integration with editor and game executables for designer workflows.

## Current State
- ✅ Modern OpenGL 4.6 with bindless textures
- ✅ Command-buffer based rendering architecture
- ✅ Per-instance material system with SSBO
- ✅ PBR physically-based rendering
- ✅ Dependency injection (no singletons)
- ✅ Instanced rendering system

## Target Architecture
```
Designer Workflow:
Asset Pipeline → Editor EXE → Material Authoring → Game EXE
                     ↓
                ECS Registry ← Graphics Framework
```

## Phase 1: Material System Enhancement (High Priority)

### 1.1 Material Serialization
**Goal:** Save/load materials from files for editor workflow

**Implementation:**
```cpp
// Material descriptor for serialization
struct MaterialDescriptor {
    std::string name;
    std::string shaderPath;
    std::unordered_map<std::string, float> floatProperties;
    std::unordered_map<std::string, glm::vec3> vec3Properties;
    std::unordered_map<std::string, std::string> texturePaths;
};

class MaterialSerializer {
public:
    static MaterialDescriptor ToDescriptor(const Material& material);
    static std::shared_ptr<Material> FromDescriptor(const MaterialDescriptor& desc);
    static bool SaveToFile(const MaterialDescriptor& desc, const std::string& path);
    static MaterialDescriptor LoadFromFile(const std::string& path);
};
```

**Files to modify:**
- `include/Resources/Material.h` - Add serialization methods
- `src/Resources/Material.cpp` - Implement serialization
- `include/Resources/MaterialSerializer.h` - New file
- `src/Resources/MaterialSerializer.cpp` - New file

### 1.2 Dynamic Shader Loading
**Goal:** Load and recompile shaders at runtime for hot-reload

**Implementation:**
```cpp
class ShaderManager {
public:
    std::shared_ptr<Shader> LoadShaderFromFile(const std::string& name,
                                              const std::string& vertPath,
                                              const std::string& fragPath);
    bool ReloadShader(const std::string& name);
    void EnableHotReload(bool enable);

private:
    std::unordered_map<std::string, ShaderFileInfo> m_ShaderFiles;
    std::unique_ptr<FileWatcher> m_FileWatcher;
};
```

**Files to modify:**
- `include/Resources/Shader.h` - Add reload capabilities
- `src/Resources/Shader.cpp` - Implement hot-reload
- `include/Resources/ShaderManager.h` - New file (extend ResourceManager)
- `src/Resources/ShaderManager.cpp` - New file

### 1.3 Material Factory System
**Goal:** Create materials from editor-generated descriptors

**Implementation:**
```cpp
class MaterialFactory {
public:
    static std::shared_ptr<Material> CreateFromDescriptor(const MaterialDescriptor& desc,
                                                         ResourceManager* resourceManager);
    static std::shared_ptr<Material> CreateFromFile(const std::string& materialPath,
                                                   ResourceManager* resourceManager);

    // Template-based material creation for common types
    static std::shared_ptr<Material> CreatePBRMaterial(const std::string& name,
                                                      std::shared_ptr<Shader> shader);
    static std::shared_ptr<Material> CreateUnlitMaterial(const std::string& name,
                                                        std::shared_ptr<Shader> shader);
};
```

**Files to create:**
- `include/Resources/MaterialFactory.h`
- `src/Resources/MaterialFactory.cpp`

## Phase 2: Editor Integration (Medium Priority)

### 2.1 Asset Pipeline Interface
**Goal:** Connect graphics framework to editor's asset management

**Implementation:**
```cpp
class IAssetProvider {
public:
    virtual ~IAssetProvider() = default;
    virtual std::vector<std::string> GetAvailableTextures() const = 0;
    virtual std::vector<std::string> GetAvailableShaders() const = 0;
    virtual std::vector<std::string> GetAvailableMaterials() const = 0;
    virtual std::string GetAssetPath(const std::string& assetName) const = 0;
};

class EditorAssetProvider : public IAssetProvider {
    // Implementation connects to editor's asset database
};

class RuntimeAssetProvider : public IAssetProvider {
    // Implementation uses packed/optimized assets
};
```

**Files to create:**
- `include/Editor/IAssetProvider.h`
- `include/Editor/EditorAssetProvider.h`
- `src/Editor/EditorAssetProvider.cpp`

### 2.2 Debug Rendering Features
**Goal:** Provide visual debugging tools for editor

**Implementation:**
```cpp
class DebugRenderer {
public:
    void EnableWireframe(bool enable);
    void EnableNormalVisualization(bool enable);
    void EnableBoundingBoxes(bool enable);
    void EnableLightVisualization(bool enable);

    void DrawWireSphere(const glm::vec3& center, float radius, const glm::vec3& color);
    void DrawWireBox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color);
    void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);

private:
    std::unique_ptr<Mesh> m_WireSphereMesh;
    std::unique_ptr<Mesh> m_WireBoxMesh;
    std::unique_ptr<Material> m_DebugMaterial;
};
```

**Files to create:**
- `include/Debug/DebugRenderer.h`
- `src/Debug/DebugRenderer.cpp`
- `assets/shaders/debug.vert`
- `assets/shaders/debug.frag`

### 2.3 Hot Reload System
**Goal:** Real-time asset reloading during editor sessions

**Implementation:**
```cpp
class HotReloadManager {
public:
    void Initialize(const std::string& assetDirectory);
    void RegisterShaderFile(const std::string& shaderName, const std::string& filePath);
    void RegisterMaterialFile(const std::string& materialName, const std::string& filePath);

    // Callbacks for when assets change
    void SetShaderReloadCallback(std::function<void(const std::string&)> callback);
    void SetMaterialReloadCallback(std::function<void(const std::string&)> callback);

private:
    std::unique_ptr<FileWatcher> m_FileWatcher;
    std::unordered_map<std::string, std::string> m_ShaderFiles;
    std::unordered_map<std::string, std::string> m_MaterialFiles;
};
```

## Phase 3: ECS Integration Points (High Priority)

### 3.1 ECS Component Integration
**Goal:** Define graphics components for ECS entities

**Implementation:**
```cpp
// Graphics components for ECS
struct MeshComponent {
    std::shared_ptr<Mesh> mesh;
    bool visible = true;
};

struct MaterialComponent {
    std::shared_ptr<Material> material;
    std::unordered_map<std::string, float> overrideFloats;
    std::unordered_map<std::string, glm::vec3> overrideVec3s;
};

struct TransformComponent {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 GetMatrix() const;
};

struct LightComponent {
    Light::Type type = Light::Type::Point;
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float range = 10.0f;
    bool enabled = true;
};
```

**Files to create:**
- `include/ECS/GraphicsComponents.h`

### 3.2 Graphics System for ECS
**Goal:** System that extracts rendering data from ECS and submits to graphics framework

**Implementation:**
```cpp
class GraphicsSystem {
public:
    GraphicsSystem(SceneRenderer* sceneRenderer);

    void Update(entt::registry& registry, const CameraComponent& camera);
    void Render();

private:
    void CollectRenderables(entt::registry& registry);
    void CollectLights(entt::registry& registry);
    void SubmitToRenderer();

    SceneRenderer* m_SceneRenderer;
    std::vector<RenderableData> m_Renderables;
    std::vector<SubmittedLightData> m_Lights;
};
```

**Files to create:**
- `include/ECS/GraphicsSystem.h`
- `src/ECS/GraphicsSystem.cpp`

### 3.3 Asset Component System
**Goal:** ECS components that reference assets by ID/path rather than direct pointers

**Implementation:**
```cpp
struct AssetMeshComponent {
    std::string meshAssetId;
    bool visible = true;
};

struct AssetMaterialComponent {
    std::string materialAssetId;
    // Runtime cache
    mutable std::shared_ptr<Material> cachedMaterial;
    mutable bool needsRefresh = true;
};

class AssetResolver {
public:
    std::shared_ptr<Mesh> ResolveMesh(const std::string& assetId);
    std::shared_ptr<Material> ResolveMaterial(const std::string& assetId);
    std::shared_ptr<Texture> ResolveTexture(const std::string& assetId);

private:
    ResourceManager* m_ResourceManager;
    IAssetProvider* m_AssetProvider;
};
```

## Phase 4: Performance & Quality Settings (Low Priority)

### 4.1 Renderer Profiles
**Goal:** Different rendering configurations for editor vs game

**Implementation:**
```cpp
enum class RendererProfile {
    Editor,      // Debug features, lower performance
    Game,        // Optimized for performance
    Preview      // Balanced for asset preview
};

class RenderProfileManager {
public:
    void SetProfile(RendererProfile profile);
    RendererProfile GetCurrentProfile() const;

    void SetEditorSettings(const EditorRenderSettings& settings);
    void SetGameSettings(const GameRenderSettings& settings);
};
```

### 4.2 Quality Settings Integration
**Goal:** LOD and quality scaling for different targets

**Implementation:**
```cpp
struct QualitySettings {
    int textureQuality = 2;    // 0=low, 1=medium, 2=high
    int shadowQuality = 2;
    float renderScale = 1.0f;
    bool enableMSAA = true;
    int maxLights = 16;
};

class QualityManager {
public:
    void ApplySettings(const QualitySettings& settings);
    void SetAutoDetect(bool enable);
};
```

## Implementation Priority Order

### Sprint 1 (Phase 1.1-1.2)
1. Material serialization system
2. Dynamic shader loading
3. Basic hot-reload for shaders

### Sprint 2 (Phase 1.3 + 3.1)
1. Material factory system
2. ECS graphics components definition
3. Asset resolver foundation

### Sprint 3 (Phase 3.2-3.3)
1. Graphics system for ECS
2. Asset component system
3. Integration testing

### Sprint 4 (Phase 2.1-2.2)
1. Asset pipeline interface
2. Debug rendering features
3. Editor-specific tools

## File Structure After Implementation

```
lib/Graphics/
├── include/
│   ├── ECS/
│   │   ├── GraphicsComponents.h
│   │   └── GraphicsSystem.h
│   ├── Editor/
│   │   ├── IAssetProvider.h
│   │   └── EditorAssetProvider.h
│   ├── Debug/
│   │   └── DebugRenderer.h
│   └── Resources/
│       ├── MaterialSerializer.h
│       ├── MaterialFactory.h
│       └── AssetResolver.h
├── src/
│   ├── ECS/
│   ├── Editor/
│   ├── Debug/
│   └── Resources/
└── assets/
    ├── shaders/
    │   ├── debug.vert
    │   └── debug.frag
    └── materials/
        └── examples/
```

## Dependencies to Add

- **JSON Library**: For material serialization (nlohmann/json)
- **File Watcher**: For hot-reload (efsw or platform-specific)
- **EnTT**: For ECS integration (already planned)

## Testing Strategy

1. **Unit Tests**: Each new system component
2. **Integration Tests**: ECS + Graphics workflow
3. **Performance Tests**: Compare editor vs game performance
4. **Hot-Reload Tests**: Asset modification scenarios

## Migration Strategy

1. **Maintain Backward Compatibility**: Current API should still work
2. **Progressive Enhancement**: Add new features without breaking existing code
3. **Feature Flags**: Allow enabling/disabling new systems
4. **Documentation**: Update examples and usage guides

---

**Next Steps:**
1. Review this roadmap with the team
2. Estimate sprint durations based on team size
3. Set up development branches for each phase
4. Begin implementation starting with Phase 1.1