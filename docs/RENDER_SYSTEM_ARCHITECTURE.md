# RenderSystem Architecture Documentation

## Overview

The RenderSystem has been refactored from a monolithic singleton into a clean, modular architecture with clear separation of concerns. This document describes the architecture after the Phase 1-5 refactoring.

---

## Architecture Diagram

```
┌──────────────────────────────────────────────────────────────┐
│                         Engine                                │
│  ┌────────────────────────────────────────────────────────┐  │
│  │           std::unique_ptr<RenderSystem>                │  │
│  └───────────────────┬────────────────────────────────────┘  │
│                      │ owns                                   │
└──────────────────────┼────────────────────────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────────────────────┐
│                    RenderSystem                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Public API:                                          │   │
│  │  • Init() / Update() / Exit()                        │   │
│  │  • SetupComponentObservers()                         │   │
│  │  • InitializeExistingEntities()                      │   │
│  │  • SetupDebugVisualization()                         │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  Owns and coordinates 6 subsystems:                          │
│                                                               │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ 1. SceneRenderer (Graphics lib, rendering pipeline)  │  │
│  └───────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ 2. Camera (Fallback camera, used if no ECS camera)   │  │
│  └───────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ 3. ShaderLibrary                                      │  │
│  │    - Load/cache shaders (PBR, shadow, picking, etc.) │  │
│  │    - Provide typed access to essential shaders       │  │
│  └───────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ 4. PrimitiveManager                                   │  │
│  │    - Generate shared primitive meshes (cube, plane)  │  │
│  │    - Generate debug visualization meshes             │  │
│  │    - Lazy creation with caching                      │  │
│  └───────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ 5. RenderResourceCache                                │  │
│  │    - Cache entity-specific meshes/materials          │  │
│  │    - Cache editor-shared meshes/materials            │  │
│  │    - Provide lookup by entity UID or resource GUID   │  │
│  └───────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ 6. ComponentInitializer                               │  │
│  │    - Setup ECS component observers                    │  │
│  │    - Initialize MeshRendererComponent resources      │  │
│  │    - Handle primitive vs asset-based meshes          │  │
│  └───────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
```

---

## Component Descriptions

### 1. RenderSystem (`engine/include/Render/Render.h`)

**Responsibilities:**
- Main entry point for rendering operations
- Coordinate subsystems
- Process ECS entities with rendering components
- Submit rendering data to SceneRenderer

**Lifecycle:**
1. **Construction**: Initialize all 6 subsystems
2. **Init()**: Setup debug visualization
3. **SetupComponentObservers()**: Register ECS observers for automatic initialization
4. **InitializeExistingEntities()**: Initialize resources for existing entities (scene load)
5. **Update()**: Per-frame rendering loop
6. **Exit()**: Clear caches
7. **Destruction**: Cleanup subsystems in correct order

**Key Methods:**
- `Update(world)`: Main rendering loop
- `SetupComponentObservers(world)`: Register EnTT observers
- `InitializeExistingEntities(world)`: Initialize loaded entities
- `SetupDebugVisualization()`: Create debug meshes

---

### 2. ShaderLibrary (`engine/include/Render/ShaderLibrary.hpp`)

**Responsibilities:**
- Load shaders from disk via ResourceManager
- Cache loaded shaders by name
- Provide typed access to essential engine shaders

**Essential Shaders:**
- `PBR Shader`: Main physically-based rendering shader
- `Shadow Shader`: Shadow depth map generation
- `Picking Shader`: Object ID encoding for entity picking
- `Primitive Shader`: Debug primitive visualization

**Key Methods:**
- `Initialize()`: Load all essential shaders
- `Load(name, vertPath, fragPath)`: Load and cache a shader
- `Get(name)`: Retrieve cached shader by name
- `GetPBRShader()`, `GetShadowShader()`, etc.: Type-safe accessors

**Dependencies:**
- ResourceManager (from SceneRenderer)

---

### 3. PrimitiveManager (`engine/include/Render/PrimitiveManager.hpp`)

**Responsibilities:**
- Generate shared primitive meshes (cube, plane, sphere)
- Generate debug visualization meshes
- Lazy creation with caching (meshes created on first request)

**Primitives:**
- **Scene Primitives**: Cube (1x1x1), Plane (2x2), Sphere (TODO)
- **Debug Primitives**: Light cube (5x5x5), Directional ray (3 units), Wireframe cube (1x1x1)

**Key Methods:**
- `Initialize()`: Pre-create common primitives
- `GetCube()`, `GetPlane()`, `GetSphere()`: Scene primitives
- `GetDebugLightCube()`, `GetDebugDirectionalRay()`, `GetDebugWireframeCube()`: Debug primitives
- `Clear()`: Release all cached meshes

**Dependencies:**
- PrimitiveGenerator (from Graphics lib)

---

### 4. RenderResourceCache (`engine/include/Render/RenderResourceCache.hpp`)

**Responsibilities:**
- Cache entity-specific rendering resources (mesh + material per entity)
- Cache editor-shared resources (mesh + material assets reused across entities)
- Provide fast lookup by entity UID or resource GUID

**Cache Types:**
1. **Entity Cache**: Maps entity UID → (mesh, material)
   - One entry per entity with MeshRendererComponent
   - Cleared when entity destroyed

2. **Editor Cache**: Maps resource GUID → mesh/material
   - Shared across multiple entities
   - Loaded from asset files or created in editor

**Key Methods:**
- `SetEntityMesh/Material(entityUID, resource)`: Cache entity-specific resource
- `GetEntityMesh/Material(entityUID)`: Lookup entity resource
- `RegisterEditorMesh/Material(guid, resource)`: Cache shared editor resource
- `GetEditorMesh/Material(guid)`: Lookup editor resource
- `ClearEntityResources(entityUID)`: Remove entity-specific cache
- `ClearAllEntityCaches()`: Clear all entity caches (scene unload)

**Dependencies:**
- None (pure data storage)

---

### 5. ComponentInitializer (`engine/include/Render/ComponentInitializer.hpp`)

**Responsibilities:**
- Setup EnTT component observers for automatic resource initialization
- Initialize rendering resources when MeshRendererComponent is added to entity
- Handle both primitive and asset-based meshes
- Coordinate with RenderResourceCache, ShaderLibrary, PrimitiveManager

**Initialization Logic:**
```
When MeshRendererComponent added:
├─ If isPrimitive == true
│  ├─ Get primitive mesh from PrimitiveManager (cube/plane)
│  └─ Create default PBR material with component.material properties
│
└─ If isPrimitive == false
   ├─ Load mesh from editor cache (via GUID)
   ├─ Load material from editor cache (via GUID)
   └─ If hasAttachedMaterial == false
      └─ Override material properties with component.material
```

**Key Methods:**
- `SetupObservers(world)`: Register EnTT on_construct observer
- `Initialize(registry, entity)`: Initialize single entity
- `InitializeAll(world)`: Initialize all existing entities (bulk operation)

**Dependencies:**
- RenderResourceCache (read/write)
- ShaderLibrary (create materials)
- PrimitiveManager (get primitive meshes)

---

## Data Flow

### Entity Creation Flow

```
1. User/Engine creates entity with MeshRendererComponent
   ↓
2. EnTT triggers on_construct observer
   ↓
3. ComponentInitializer::Initialize() called
   ↓
4. Determine if primitive or asset-based
   ↓
5. Load/create mesh resource
   ↓
6. Load/create material resource
   ↓
7. Cache resources in RenderResourceCache (entity UID → resources)
   ↓
8. Done (entity ready for rendering)
```

### Rendering Flow (Per Frame)

```
1. RenderSystem::Update(world) called by Engine
   ↓
2. Clear frame buffer (SceneRenderer)
   ↓
3. Update camera from ECS CameraComponent (if exists)
   ↓
4. Query all entities with (MeshRendererComponent, TransformComponent, VisibilityComponent)
   ↓
5. For each entity:
   ├─ Get entityUID
   ├─ Lookup mesh from RenderResourceCache
   ├─ Lookup material from RenderResourceCache
   ├─ Create RenderableData struct
   └─ Submit to SceneRenderer
   ↓
6. Query all entities with (LightComponent, PositionComponent)
   ↓
7. For each light:
   ├─ Create SubmittedLightData struct
   └─ Submit to SceneRenderer
   ↓
8. SceneRenderer->Render()
   ├─ Shadow pass
   ├─ Main PBR pass
   ├─ Picking pass (if enabled)
   ├─ Debug pass
   └─ Post-process pass
   ↓
9. Done (frame rendered)
```

### Scene Load Flow

```
1. Engine loads scene YAML
   ↓
2. ECS world populated with entities
   ↓
3. RenderSystem::SetupComponentObservers(world)
   ├─ Register on_construct observers
   └─ Future entities automatically initialized
   ↓
4. RenderSystem::InitializeExistingEntities(world)
   ├─ Query all entities with MeshRendererComponent
   ├─ Call ComponentInitializer::Initialize() for each
   └─ Populate RenderResourceCache for all entities
   ↓
5. Done (scene ready for rendering)
```

---

## Refactoring History

### Before Refactoring (Phases 0-5)

**Problems:**
- Monolithic RenderSystem class (~420 lines)
- 6 file-scope global variables (mutable state)
- Static singleton pattern (hidden dependencies)
- Shader loading mixed with rendering logic
- Primitive generation mixed with rendering logic
- Component initialization mixed with rendering logic
- Hard to test, hard to reason about lifetimes

### After Refactoring (Phases 1-6)

**Improvements:**
- ✅ Clear separation of concerns (6 focused classes)
- ✅ No file-scope variables (all state owned by classes)
- ✅ No singleton pattern (Engine owns RenderSystem)
- ✅ Explicit ownership and dependencies
- ✅ Each class < 300 lines
- ✅ Testable components
- ✅ Clear lifecycle management

---

## Phase-by-Phase Changes

| Phase | Goal | Changes |
|-------|------|---------|
| **Phase 0** | Preparation | Documented current API, identified call sites |
| **Phase 1** | Extract ShaderLibrary | Created ShaderLibrary class, removed shader loading from RenderSystem |
| **Phase 2** | Extract PrimitiveManager | Created PrimitiveManager class, removed primitive generation from RenderSystem |
| **Phase 3** | Extract RenderResourceCache | Created RenderResourceCache class, removed 4 file-scope cache variables |
| **Phase 4** | Extract ComponentInitializer | Created ComponentInitializer class, separated initialization logic from rendering |
| **Phase 5** | Remove Singleton Pattern | Removed InstanceData, removed static methods, Engine owns RenderSystem |
| **Phase 6** | Documentation & Cleanup | Added Doxygen comments, architecture docs, removed remaining file-scope variables |

---

## Usage Examples

### Example 1: Engine Initialization

```cpp
// In Engine::Init()
void Engine::Init(std::string const& cfg) {
    // ... other initialization ...

    // Create RenderSystem (constructor initializes all subsystems)
    Instance().m_RenderSystem = std::make_unique<RenderSystem>();

    // Complete initialization
    Instance().m_RenderSystem->Init();

    // Setup observers and initialize existing entities
    Instance().m_RenderSystem->SetupComponentObservers(Instance().m_World);
    Instance().m_RenderSystem->InitializeExistingEntities(Instance().m_World);
}
```

### Example 2: Per-Frame Rendering

```cpp
// In Engine::Update()
void Engine::Update() {
    Engine& instance = Instance();
    while (instance.m_Info.m_State == Info::State::Running) {
        // Poll input, update game logic, etc.
        // ...

        // Render frame
        instance.m_RenderSystem->Update(instance.m_World);

        // Swap buffers, update debug, etc.
        // ...
    }
}
```

### Example 3: Creating a Renderable Entity

```cpp
// Create entity with rendering components
ecs::world world = Engine::GetWorld();
auto entity = world.add_entity();

// Add transform components
world.add_component_to_entity<PositionComponent>(entity, glm::vec3(0, 0, 0));
world.add_component_to_entity<TransformComponent>(entity, glm::mat4(1.0f));
world.add_component_to_entity<VisibilityComponent>(entity, true);

// Add mesh renderer (observer automatically initializes resources)
MeshRendererComponent meshRenderer;
meshRenderer.isPrimitive = true;
meshRenderer.m_PrimitiveType = MeshRendererComponent::PrimitiveType::CUBE;
meshRenderer.material.m_AlbedoColor = glm::vec3(1.0f, 0.0f, 0.0f);
meshRenderer.material.metallic = 0.0f;
meshRenderer.material.roughness = 0.6f;

world.add_component_to_entity<MeshRendererComponent>(entity, meshRenderer);

// Entity is now ready for rendering!
```

### Example 4: Loading Asset-Based Mesh

```cpp
// Load mesh and material assets in editor
Resource::Guid meshGuid = /* loaded from file */;
Resource::Guid materialGuid = /* loaded from file */;

std::shared_ptr<Mesh> mesh = /* load mesh */;
std::shared_ptr<Material> material = /* load material */;

// Register with RenderSystem for reuse
RenderSystem::RegisterEditorMesh(meshGuid, mesh);
RenderSystem::RegisterEditorMaterial(materialGuid, material);

// Create entity using registered assets
MeshRendererComponent meshRenderer;
meshRenderer.isPrimitive = false;
meshRenderer.m_MeshGuid = meshGuid;
meshRenderer.m_MaterialGuid = materialGuid;

world.add_component_to_entity<MeshRendererComponent>(entity, meshRenderer);
```

### Example 5: Scene Load

```cpp
// Load scene from YAML
ecs::world world = Engine::GetWorld();
world.UnloadAll();
world.LoadYAML("path/to/scene.yaml");

// Setup RenderSystem for loaded scene
auto& renderSystem = Engine::GetRenderSystem();
renderSystem.SetupComponentObservers(world);      // Register observers for future entities
renderSystem.InitializeExistingEntities(world);   // Initialize all loaded entities

// Scene is ready for rendering
```

---

## Testing Considerations

### Unit Testing Individual Subsystems

Each subsystem can be tested in isolation:

```cpp
// Test ShaderLibrary
TEST(ShaderLibraryTest, LoadPBRShader) {
    ResourceManager resourceManager;
    ShaderLibrary shaderLibrary(&resourceManager);

    ASSERT_TRUE(shaderLibrary.Initialize());
    ASSERT_NE(shaderLibrary.GetPBRShader(), nullptr);
}

// Test PrimitiveManager
TEST(PrimitiveManagerTest, CreateCube) {
    PrimitiveManager primitiveManager;
    auto cube = primitiveManager.GetCube();

    ASSERT_NE(cube, nullptr);
    ASSERT_FALSE(cube->vertices.empty());
}

// Test RenderResourceCache
TEST(RenderResourceCacheTest, CacheEntityMesh) {
    RenderResourceCache cache;
    auto mesh = std::make_shared<Mesh>(/* ... */);

    cache.SetEntityMesh(123, mesh);
    ASSERT_EQ(cache.GetEntityMesh(123), mesh);
}
```

### Integration Testing RenderSystem

Test the full system with a mock ECS world:

```cpp
TEST(RenderSystemTest, InitializeAndRender) {
    // Create minimal world
    ecs::world world = WorldRegistry::NewWorld();

    // Create entity with mesh renderer
    auto entity = world.add_entity();
    world.add_component_to_entity<MeshRendererComponent>(entity, /* ... */);
    world.add_component_to_entity<TransformComponent>(entity, glm::mat4(1.0f));
    world.add_component_to_entity<VisibilityComponent>(entity, true);

    // Initialize RenderSystem
    RenderSystem renderSystem;
    renderSystem.Init();
    renderSystem.SetupComponentObservers(world);
    renderSystem.InitializeExistingEntities(world);

    // Should not crash
    ASSERT_NO_THROW(renderSystem.Update(world));
}
```

---

## Performance Considerations

### Resource Caching Strategy

1. **Entity Resources**: Initialized once per entity, reused every frame
   - Cache key: Entity UID
   - Invalidation: Entity destroyed or components changed

2. **Editor Resources**: Loaded once, reused across multiple entities
   - Cache key: Resource GUID
   - Invalidation: Asset reload or scene unload

3. **Primitive Meshes**: Created once, shared across all entities using that primitive
   - No per-entity overhead
   - Lazy creation (only created if requested)

### Memory Usage

- **Before Refactoring**: ~6 file-scope std::unordered_maps (unbounded growth)
- **After Refactoring**: Explicit cache management with clear ownership
  - Entity caches cleared on entity destruction
  - Editor caches cleared on scene unload
  - Primitive caches persist (small, fixed size)

### Frame Time

- **No per-frame allocation**: Resources cached during initialization
- **Fast lookup**: O(1) hash map lookup by entity UID
- **No redundant work**: Material properties set once during initialization, not per-frame

---

## Future Improvements

### Potential Enhancements

1. **Hot Reload**: Shader/material hot-reloading support
2. **Threading**: Parallel entity processing in Update()
3. **LOD System**: Level-of-detail mesh selection
4. **Material Instancing**: GPU instancing for identical materials
5. **Occlusion Culling**: CPU-side occlusion query system
6. **Async Resource Loading**: Load meshes/materials on worker threads

### Performance Profiling

Recommended profiling points:
- `RenderSystem::Update()` - Total frame time
- `ComponentInitializer::Initialize()` - Initialization overhead
- `SceneRenderer::Render()` - GPU rendering time
- `RenderResourceCache lookups` - Cache hit/miss rates

---

## Conclusion

The refactored RenderSystem architecture provides:
- ✅ Clear separation of concerns
- ✅ Explicit ownership and lifetimes
- ✅ Testable components
- ✅ No global mutable state
- ✅ Easy to extend and maintain

This architecture supports future growth while maintaining code quality and performance.
