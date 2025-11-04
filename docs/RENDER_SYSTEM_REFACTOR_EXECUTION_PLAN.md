# RenderSystem Refactoring Execution Plan

## Overview

This plan refactors the RenderSystem from a monolithic class with mixed responsibilities into a clean, maintainable architecture with clear separation of concerns.

**Goals:**
- Clear single responsibilities per class
- No static singletons or global state
- Explicit ownership and lifecycle
- Testable components
- Maintainable codebase

**Strategy:** Incremental refactoring - system remains functional after each phase.

---

## Current State Analysis

### File: `engine/src/Render/Render.cpp`

**Lines of Code:** ~420 lines
**Responsibilities:** 7 different concerns mixed together
**Global State:** 6 file-scope static variables
**Static Methods:** 10+ static functions

### Problems Identified

| Problem | Lines | Impact |
|---------|-------|--------|
| File-scope caches | 24-29, 268-270, 378-380 | Global mutable state |
| Static singleton pattern | 233-254 | Hidden dependencies |
| Shader loading in system | 290-348 | Wrong responsibility |
| Primitive generation in system | 272-288, 351-369 | Wrong responsibility |
| Material creation in initializer | 127-130 | No clear factory |
| Mixed initialization/rendering | 59-172 | Hard to test |
| Component observers in system | 148-172 | Coupling |

---

## Refactoring Phases

### Phase 0: Preparation (30 min)
**Goal:** Create clean slate for new architecture

**Tasks:**
1. Document current RenderSystem interface
2. Identify all call sites in Engine/Editor
3. Create backup branch
4. Write integration test for current behavior

**Deliverables:**
- `RENDER_SYSTEM_CURRENT_API.md` - Current interface documentation
- `RENDER_SYSTEM_CALLSITES.md` - All usage locations
- Git branch: `refactor/render-system-prep`

---

### Phase 1: Extract ShaderLibrary (2-3 hours)
**Goal:** Centralize shader management

**Why First:** Independent, no dependencies on other systems

#### Step 1.1: Create ShaderLibrary Class

**File:** `engine/include/Render/ShaderLibrary.hpp`

```cpp
#ifndef ENGINE_SHADER_LIBRARY_HPP
#define ENGINE_SHADER_LIBRARY_HPP
#pragma once

#include <Resources/Shader.h>
#include <Resources/ResourceManager.h>
#include <memory>
#include <unordered_map>
#include <string>

/**
 * @brief Manages shader loading and caching for the render system
 *
 * Responsibilities:
 * - Load shaders from disk via ResourceManager
 * - Cache loaded shaders by name
 * - Provide typed access to common shaders
 */
class ShaderLibrary {
public:
    explicit ShaderLibrary(ResourceManager* resourceManager);
    ~ShaderLibrary() = default;

    // Delete copy/move
    ShaderLibrary(const ShaderLibrary&) = delete;
    ShaderLibrary& operator=(const ShaderLibrary&) = delete;

    /**
     * @brief Initialize and load essential shaders
     * @return true if all essential shaders loaded successfully
     */
    bool Initialize();

    /**
     * @brief Get the main PBR shader for scene rendering
     */
    std::shared_ptr<Shader> GetPBRShader() const { return m_PBRShader; }

    /**
     * @brief Get shader for shadow depth rendering
     */
    std::shared_ptr<Shader> GetShadowShader() const { return m_ShadowShader; }

    /**
     * @brief Get shader for object picking
     */
    std::shared_ptr<Shader> GetPickingShader() const { return m_PickingShader; }

    /**
     * @brief Get shader for debug primitive rendering
     */
    std::shared_ptr<Shader> GetPrimitiveShader() const { return m_PrimitiveShader; }

    /**
     * @brief Load a shader by name
     * @param name Unique shader identifier
     * @param vertPath Path to vertex shader
     * @param fragPath Path to fragment shader
     * @return Shader pointer, or nullptr on failure
     */
    std::shared_ptr<Shader> Load(
        const std::string& name,
        const std::string& vertPath,
        const std::string& fragPath
    );

    /**
     * @brief Get a previously loaded shader by name
     */
    std::shared_ptr<Shader> Get(const std::string& name) const;

    /**
     * @brief Check if shader exists
     */
    bool Has(const std::string& name) const;

    /**
     * @brief Clear all cached shaders
     */
    void Clear();

private:
    ResourceManager* m_ResourceManager;

    // Essential shaders for engine
    std::shared_ptr<Shader> m_PBRShader;
    std::shared_ptr<Shader> m_ShadowShader;
    std::shared_ptr<Shader> m_PickingShader;
    std::shared_ptr<Shader> m_PrimitiveShader;

    // Named shader cache
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;

    // Load essential shaders
    bool LoadPBRShader();
    bool LoadShadowShader();
    bool LoadPickingShader();
    bool LoadPrimitiveShader();
};

#endif // ENGINE_SHADER_LIBRARY_HPP
```

#### Step 1.2: Implement ShaderLibrary

**File:** `engine/src/Render/ShaderLibrary.cpp`

```cpp
#include "Render/ShaderLibrary.hpp"
#include <spdlog/spdlog.h>
#include <cassert>

ShaderLibrary::ShaderLibrary(ResourceManager* resourceManager)
    : m_ResourceManager(resourceManager)
{
    assert(resourceManager && "ResourceManager cannot be null");
}

bool ShaderLibrary::Initialize() {
    spdlog::info("ShaderLibrary: Initializing...");

    bool success = true;
    success &= LoadPBRShader();
    success &= LoadShadowShader();
    success &= LoadPickingShader();
    success &= LoadPrimitiveShader();

    if (success) {
        spdlog::info("ShaderLibrary: All essential shaders loaded successfully");
    } else {
        spdlog::error("ShaderLibrary: Failed to load some essential shaders");
    }

    return success;
}

std::shared_ptr<Shader> ShaderLibrary::Load(
    const std::string& name,
    const std::string& vertPath,
    const std::string& fragPath)
{
    // Check if already loaded
    if (m_Shaders.contains(name)) {
        spdlog::debug("ShaderLibrary: Shader '{}' already loaded", name);
        return m_Shaders[name];
    }

    // Load via ResourceManager
    auto shader = m_ResourceManager->LoadShader(name, vertPath, fragPath);
    if (shader) {
        m_Shaders[name] = shader;
        spdlog::debug("ShaderLibrary: Loaded shader '{}'", name);
    } else {
        spdlog::error("ShaderLibrary: Failed to load shader '{}'", name);
    }

    return shader;
}

std::shared_ptr<Shader> ShaderLibrary::Get(const std::string& name) const {
    auto it = m_Shaders.find(name);
    return (it != m_Shaders.end()) ? it->second : nullptr;
}

bool ShaderLibrary::Has(const std::string& name) const {
    return m_Shaders.contains(name);
}

void ShaderLibrary::Clear() {
    m_Shaders.clear();
    m_PBRShader.reset();
    m_ShadowShader.reset();
    m_PickingShader.reset();
    m_PrimitiveShader.reset();
    spdlog::info("ShaderLibrary: Cleared all shaders");
}

bool ShaderLibrary::LoadPBRShader() {
    m_PBRShader = Load("main_pbr",
        "assets/shaders/main_pbr.vert",
        "assets/shaders/main_pbr.frag");

    if (!m_PBRShader) {
        spdlog::error("ShaderLibrary: Failed to load PBR shader (CRITICAL)");
        return false;
    }

    return true;
}

bool ShaderLibrary::LoadShadowShader() {
    m_ShadowShader = Load("shadow_depth",
        "assets/shaders/shadow_depth.vert",
        "assets/shaders/shadow_depth.frag");

    if (!m_ShadowShader) {
        spdlog::warn("ShaderLibrary: Failed to load shadow shader (non-critical)");
        return false;
    }

    return true;
}

bool ShaderLibrary::LoadPickingShader() {
    m_PickingShader = Load("picking",
        "assets/shaders/picking.vert",
        "assets/shaders/picking.frag");

    if (!m_PickingShader) {
        spdlog::warn("ShaderLibrary: Failed to load picking shader (non-critical)");
        return false;
    }

    return true;
}

bool ShaderLibrary::LoadPrimitiveShader() {
    m_PrimitiveShader = Load("primitive",
        "assets/shaders/primitive.vert",
        "assets/shaders/primitive.frag");

    if (!m_PrimitiveShader) {
        spdlog::warn("ShaderLibrary: Failed to load primitive shader (non-critical)");
        return false;
    }

    return true;
}
```

#### Step 1.3: Integrate ShaderLibrary into RenderSystem

**File:** `engine/include/Render/Render.h`

```cpp
// Add forward declaration
class ShaderLibrary;

struct RenderSystem : public ecs::SystemBase {
    // ... existing code ...

    struct InstanceData {
        std::unique_ptr<SceneRenderer> m_SceneRenderer;
        std::unique_ptr<Camera> m_Camera;
        std::unique_ptr<ShaderLibrary> m_ShaderLibrary;  // NEW

        void Acquire();
        void Release();
    };

    // Remove: static std::shared_ptr<Shader> s_CubeShader;
    // Replace with: GetShaderLibrary()->GetPBRShader()
};
```

#### Step 1.4: Update RenderSystem Implementation

**File:** `engine/src/Render/Render.cpp`

```cpp
// Remove LoadBasicShaders() function (lines 290-348)
// Remove static s_CubeShader (line 33)

void RenderSystem::InstanceData::Acquire() {
    m_SceneRenderer = std::make_unique<SceneRenderer>();
    m_Camera = std::make_unique<Camera>(CameraType::Perspective);
    m_Camera->SetPerspective(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    m_Camera->SetPosition(glm::vec3(0.0f, 2.0f, 8.0f));
    m_Camera->SetRotation(glm::vec3(-10.0f, 0.0f, 0.0f));

    // NEW: Initialize ShaderLibrary
    m_ShaderLibrary = std::make_unique<ShaderLibrary>(
        m_SceneRenderer->GetResourceManager()
    );

    if (!m_ShaderLibrary->Initialize()) {
        spdlog::error("Failed to initialize ShaderLibrary");
    }

    // Configure SceneRenderer with shaders
    m_SceneRenderer->SetShadowDepthShader(m_ShaderLibrary->GetShadowShader());
    m_SceneRenderer->SetPickingShader(m_ShaderLibrary->GetPickingShader());
    m_SceneRenderer->SetDebugPrimitiveShader(m_ShaderLibrary->GetPrimitiveShader());
}

void RenderSystem::InstanceData::Release() {
    m_ShaderLibrary.reset();  // NEW
    m_SceneRenderer.reset();
    m_Camera.reset();
}

void RenderSystem::Init() {
    Instance().Acquire();
    SetupDebugVisualization();  // LoadBasicShaders() removed

    spdlog::info("RenderSystem initialized");
    spdlog::info("RenderSystem: Call SetupComponentObservers() and InitializeExistingEntities() after world is created");
}
```

#### Step 1.5: Update Material Creation

**File:** `engine/src/Render/Render.cpp` (in InitializeMeshRenderer)

```cpp
// Line 122-125: Replace s_CubeShader with ShaderLibrary
if (!RenderSystem::Instance().m_ShaderLibrary) {
    spdlog::error("RenderSystem: ShaderLibrary not initialized");
    return;
}

auto pbrShader = RenderSystem::Instance().m_ShaderLibrary->GetPBRShader();
if (!pbrShader) {
    spdlog::error("RenderSystem: PBR shader not available");
    return;
}

materialResource = std::make_shared<Material>(
    pbrShader,
    "DefaultMaterial_Entity_" + std::to_string(entityUID)
);
```

#### Step 1.6: Update CMakeLists.txt

**File:** `engine/CMakeLists.txt`

```cmake
# Add new files
set(ENGINE_SOURCES
    # ... existing sources ...
    src/Render/ShaderLibrary.cpp
)

set(ENGINE_HEADERS
    # ... existing headers ...
    include/Render/ShaderLibrary.hpp
)
```

#### Step 1.7: Test Phase 1

**Test checklist:**
- [ ] Engine compiles without errors
- [ ] Shaders load correctly on startup
- [ ] Entities render with correct PBR shader
- [ ] Shadow mapping works
- [ ] Object picking works
- [ ] Debug primitives render
- [ ] No static shader globals remain

**Deliverables:**
- ShaderLibrary class (hpp + cpp)
- Updated RenderSystem integration
- All tests passing
- Commit: `refactor: Extract ShaderLibrary from RenderSystem`

---

### Phase 2: Extract PrimitiveManager (2-3 hours)
**Goal:** Manage shared primitive meshes

**Why Second:** Independent, no dependencies except ShaderLibrary

#### Step 2.1: Create PrimitiveManager Class

**File:** `engine/include/Render/PrimitiveManager.hpp`

```cpp
#ifndef ENGINE_PRIMITIVE_MANAGER_HPP
#define ENGINE_PRIMITIVE_MANAGER_HPP
#pragma once

#include <Resources/Mesh.h>
#include <memory>

/**
 * @brief Manages shared primitive mesh instances
 *
 * Responsibilities:
 * - Lazy-create primitive meshes (cube, plane, sphere, etc.)
 * - Ensure primitives are only created once
 * - Provide access to shared mesh instances for memory efficiency
 */
class PrimitiveManager {
public:
    PrimitiveManager() = default;
    ~PrimitiveManager() = default;

    // Delete copy/move
    PrimitiveManager(const PrimitiveManager&) = delete;
    PrimitiveManager& operator=(const PrimitiveManager&) = delete;

    /**
     * @brief Pre-create common primitives (optional optimization)
     */
    void Initialize();

    /**
     * @brief Get shared cube mesh (1x1x1)
     * @return Cube mesh, created on first call
     */
    std::shared_ptr<Mesh> GetCube();

    /**
     * @brief Get shared plane mesh (2x2)
     * @return Plane mesh, created on first call
     */
    std::shared_ptr<Mesh> GetPlane();

    /**
     * @brief Get shared sphere mesh
     * @return Sphere mesh, created on first call
     */
    std::shared_ptr<Mesh> GetSphere();

    /**
     * @brief Get debug light visualization cube (5x5x5)
     * @return Debug cube mesh, created on first call
     */
    std::shared_ptr<Mesh> GetDebugLightCube();

    /**
     * @brief Get debug directional light ray mesh
     * @return Ray mesh, created on first call
     */
    std::shared_ptr<Mesh> GetDebugDirectionalRay();

    /**
     * @brief Get debug wireframe cube for AABB visualization
     * @return Wireframe cube mesh, created on first call
     */
    std::shared_ptr<Mesh> GetDebugWireframeCube();

    /**
     * @brief Clear all cached primitives
     */
    void Clear();

private:
    // Scene primitives
    std::shared_ptr<Mesh> m_Cube;
    std::shared_ptr<Mesh> m_Plane;
    std::shared_ptr<Mesh> m_Sphere;

    // Debug visualization primitives
    std::shared_ptr<Mesh> m_DebugLightCube;
    std::shared_ptr<Mesh> m_DebugDirectionalRay;
    std::shared_ptr<Mesh> m_DebugWireframeCube;
};

#endif // ENGINE_PRIMITIVE_MANAGER_HPP
```

#### Step 2.2: Implement PrimitiveManager

**File:** `engine/src/Render/PrimitiveManager.cpp`

```cpp
#include "Render/PrimitiveManager.hpp"
#include <Resources/PrimitiveGenerator.h>
#include <spdlog/spdlog.h>
#include <cassert>

void PrimitiveManager::Initialize() {
    spdlog::info("PrimitiveManager: Pre-creating common primitives");

    // Pre-create common primitives for faster first access
    GetCube();
    GetPlane();
    GetDebugLightCube();
    GetDebugDirectionalRay();
    GetDebugWireframeCube();

    spdlog::info("PrimitiveManager: Initialized with 5 primitives");
}

std::shared_ptr<Mesh> PrimitiveManager::GetCube() {
    if (!m_Cube) {
        m_Cube = std::make_shared<Mesh>(PrimitiveGenerator::CreateCube(1.0f));
        assert(m_Cube && "Cube mesh generation failed");
        assert(!m_Cube->vertices.empty() && "Generated cube has no vertices");
        spdlog::debug("PrimitiveManager: Created cube mesh");
    }
    return m_Cube;
}

std::shared_ptr<Mesh> PrimitiveManager::GetPlane() {
    if (!m_Plane) {
        m_Plane = std::make_shared<Mesh>(PrimitiveGenerator::CreatePlane(2.0f, 2.0f, 1, 1));
        assert(m_Plane && "Plane mesh generation failed");
        assert(!m_Plane->vertices.empty() && "Generated plane has no vertices");
        spdlog::debug("PrimitiveManager: Created plane mesh");
    }
    return m_Plane;
}

std::shared_ptr<Mesh> PrimitiveManager::GetSphere() {
    if (!m_Sphere) {
        // TODO: Implement sphere generation
        spdlog::warn("PrimitiveManager: Sphere generation not yet implemented");
    }
    return m_Sphere;
}

std::shared_ptr<Mesh> PrimitiveManager::GetDebugLightCube() {
    if (!m_DebugLightCube) {
        m_DebugLightCube = std::make_shared<Mesh>(PrimitiveGenerator::CreateCube(5.0f));
        assert(m_DebugLightCube && "Debug light cube generation failed");
        spdlog::debug("PrimitiveManager: Created debug light cube");
    }
    return m_DebugLightCube;
}

std::shared_ptr<Mesh> PrimitiveManager::GetDebugDirectionalRay() {
    if (!m_DebugDirectionalRay) {
        m_DebugDirectionalRay = std::make_shared<Mesh>(
            PrimitiveGenerator::CreateDirectionalRay(3.0f)
        );
        assert(m_DebugDirectionalRay && "Debug directional ray generation failed");
        spdlog::debug("PrimitiveManager: Created debug directional ray");
    }
    return m_DebugDirectionalRay;
}

std::shared_ptr<Mesh> PrimitiveManager::GetDebugWireframeCube() {
    if (!m_DebugWireframeCube) {
        m_DebugWireframeCube = std::make_shared<Mesh>(
            PrimitiveGenerator::CreateWireframeCube(1.0f)
        );
        assert(m_DebugWireframeCube && "Debug wireframe cube generation failed");
        spdlog::debug("PrimitiveManager: Created debug wireframe cube");
    }
    return m_DebugWireframeCube;
}

void PrimitiveManager::Clear() {
    m_Cube.reset();
    m_Plane.reset();
    m_Sphere.reset();
    m_DebugLightCube.reset();
    m_DebugDirectionalRay.reset();
    m_DebugWireframeCube.reset();
    spdlog::info("PrimitiveManager: Cleared all primitives");
}
```

#### Step 2.3: Integration Steps (Similar to Phase 1)

1. Add `m_PrimitiveManager` to `RenderSystem::InstanceData`
2. Remove file-scope `g_SharedCubeMesh`, `g_SharedPlaneMesh`
3. Remove `GetSharedCubeMesh()`, `GetSharedPlaneMesh()` static methods
4. Replace calls with `Instance().m_PrimitiveManager->GetCube()`
5. Update `SetupDebugVisualization()` to use PrimitiveManager
6. Update CMakeLists.txt
7. Test

**Deliverables:**
- PrimitiveManager class
- Updated RenderSystem integration
- Commit: `refactor: Extract PrimitiveManager from RenderSystem`

---

### Phase 3: Extract RenderResourceCache (3-4 hours)
**Goal:** Centralize resource caching

**File:** `engine/include/Render/RenderResourceCache.hpp`

```cpp
// Similar structure to what I tried to write earlier
// Manages g_EntityMeshCache, g_EntityMaterialCache
// Manages g_EditorMeshCache, g_EditorMaterialCache
```

**Migration:**
1. Create RenderResourceCache class
2. Move 4 file-scope caches into class members
3. Move RegisterEditorMesh/Material to cache methods
4. Move ClearEntityResources/ClearAllCaches to cache methods
5. Add to InstanceData
6. Update all access points

**Deliverables:**
- RenderResourceCache class
- No file-scope caches remain
- Commit: `refactor: Extract RenderResourceCache from RenderSystem`

---

### Phase 4: Extract ComponentInitializer (4-5 hours)
**Goal:** Separate initialization logic from rendering

**File:** `engine/include/Render/ComponentInitializer.hpp`

```cpp
// Responsibilities:
// - InitializeMeshRenderer logic
// - SetupComponentObservers logic
// - InitializeExistingEntities logic

class ComponentInitializer {
public:
    ComponentInitializer(
        RenderResourceCache& cache,
        ShaderLibrary& shaderLibrary,
        PrimitiveManager& primitiveManager
    );

    void Initialize(entt::registry& registry, entt::entity entity);
    void InitializeAll(ecs::world& world);
    void SetupObservers(ecs::world& world);
};
```

**Migration:**
1. Create ComponentInitializer class
2. Move InitializeMeshRenderer to ComponentInitializer::Initialize
3. Move SetupComponentObservers to ComponentInitializer::SetupObservers
4. Move InitializeExistingEntities to ComponentInitializer::InitializeAll
5. Update RenderSystem to own ComponentInitializer
6. Update call sites

**Deliverables:**
- ComponentInitializer class
- RenderSystem only handles rendering (not initialization)
- Commit: `refactor: Extract ComponentInitializer from RenderSystem`

---

### Phase 5: Remove Static Singleton Pattern (3-4 hours)
**Goal:** Make RenderSystem a normal class

**Current Pattern:**
```cpp
static RenderSystem::InstanceData& Instance();
static std::unique_ptr<InstanceData>& InstancePtr();
static RenderSystem System();
```

**Target Pattern:**
```cpp
// RenderSystem is now a regular class
class RenderSystem {
    // Direct members instead of InstanceData
    std::unique_ptr<SceneRenderer> m_SceneRenderer;
    std::unique_ptr<Camera> m_Camera;
    std::unique_ptr<ShaderLibrary> m_ShaderLibrary;
    // ...
};

// Engine owns instance
class Engine {
    std::unique_ptr<RenderSystem> m_RenderSystem;
};
```

**Migration Steps:**

1. **Move InstanceData members to RenderSystem directly**
   ```cpp
   // Remove nested InstanceData struct
   // Move members to RenderSystem private section
   ```

2. **Remove static singleton methods**
   ```cpp
   // Remove: Instance(), InstancePtr(), System()
   ```

3. **Update Engine to own RenderSystem**
   ```cpp
   // In Engine::Init()
   m_RenderSystem = std::make_unique<RenderSystem>();
   m_RenderSystem->Init();

   // In Engine::Update()
   m_RenderSystem->Update(m_World);

   // In Engine::Exit()
   m_RenderSystem.reset();
   ```

4. **Update all RenderSystem call sites**
   ```cpp
   // Before: RenderSystem::System().Init()
   // After:  Engine::GetRenderSystem()->Init()

   // Or pass reference where needed
   ```

5. **Make constructors/destructors normal**
   ```cpp
   RenderSystem();   // Not private
   ~RenderSystem();  // Normal destructor
   ```

**Deliverables:**
- RenderSystem as normal class (no singleton)
- Engine owns RenderSystem instance
- No static state in RenderSystem
- Commit: `refactor: Remove singleton pattern from RenderSystem`

---

### Phase 6: Final Cleanup & Documentation (2-3 hours)
**Goal:** Polish and document

**Tasks:**
1. Remove all remaining file-scope variables
2. Add doxygen comments to all public methods
3. Update RENDER_SYSTEM_USAGE.md
4. Create architecture diagram
5. Add unit tests for extracted classes
6. Performance profiling comparison
7. Update contribution guide

**Deliverables:**
- Complete documentation
- Architecture diagram
- Unit tests
- Performance report
- Commit: `refactor: Finalize RenderSystem refactoring and add documentation`

---

## Timeline Summary

| Phase | Duration | Cumulative |
|-------|----------|------------|
| Phase 0: Preparation | 0.5h | 0.5h |
| Phase 1: ShaderLibrary | 2-3h | 2.5-3.5h |
| Phase 2: PrimitiveManager | 2-3h | 4.5-6.5h |
| Phase 3: RenderResourceCache | 3-4h | 7.5-10.5h |
| Phase 4: ComponentInitializer | 4-5h | 11.5-15.5h |
| Phase 5: Remove Singleton | 3-4h | 14.5-19.5h |
| Phase 6: Cleanup | 2-3h | 16.5-22.5h |
| **Total** | **17-23 hours** | **~3 days** |

---

## Testing Strategy

### Per-Phase Testing

After each phase, verify:
- [ ] Engine compiles without errors
- [ ] Engine runs without crashes
- [ ] Entities render correctly
- [ ] Materials display correct colors
- [ ] Lights work properly
- [ ] No visual regressions
- [ ] Performance is same or better

### Integration Testing

After all phases:
- [ ] Create new world with entities
- [ ] Load existing world
- [ ] Add/remove entities at runtime
- [ ] Change materials at runtime
- [ ] Hot-reload shaders
- [ ] Stress test (1000+ entities)

---

## Risk Mitigation

### High-Risk Areas

1. **Phase 5 (Singleton Removal):** Many call sites to update
   - Mitigation: Search for all `RenderSystem::` calls first
   - Create helper functions if needed

2. **Phase 4 (ComponentInitializer):** EnTT observer setup
   - Mitigation: Test observer functionality thoroughly
   - Keep backup of working observer code

3. **Phase 3 (Resource Cache):** Memory management
   - Mitigation: Use smart pointers consistently
   - Add cache statistics logging

### Rollback Plan

Each phase is a separate commit. If issues arise:
```bash
git revert <commit-hash>  # Revert specific phase
git checkout <previous-phase-branch>  # Full rollback
```

---

## Success Criteria

### Code Quality Metrics

- [ ] No file-scope static variables
- [ ] No static singleton pattern
- [ ] Each class < 300 lines
- [ ] Each class has single responsibility
- [ ] All public methods documented
- [ ] No circular dependencies

### Performance Metrics

- [ ] Frame time ≤ current implementation
- [ ] Memory usage ≤ current implementation
- [ ] Startup time ≤ current implementation

### Maintainability Metrics

- [ ] New developer can understand class purposes
- [ ] Easy to add new primitive types
- [ ] Easy to add new shader types
- [ ] Easy to write unit tests
- [ ] Clear ownership of resources

---

## Post-Refactoring Benefits

### Immediate Benefits

✅ **Testability:** Each class can be unit tested in isolation
✅ **Debuggability:** Clear boundaries make debugging easier
✅ **Maintainability:** Single responsibility = easier changes
✅ **Documentation:** Smaller classes = better docs

### Future Benefits

✅ **Hot Reload:** Easier to reload shaders/materials
✅ **Threading:** Clear boundaries enable parallel rendering
✅ **Editor Integration:** Cleaner API for editor to use
✅ **Performance:** Easier to profile and optimize

---

## Getting Started

### Prerequisites

1. Current code compiles and runs
2. Git branch for refactoring
3. Backup of current working state
4. Time allocation (~3 days of focused work)

### First Steps

```bash
# 1. Create refactoring branch
git checkout -b refactor/render-system
git checkout -b refactor/render-system-prep

# 2. Document current API
# (Do Phase 0 tasks)

# 3. Start with Phase 1
# (ShaderLibrary extraction)

# 4. Commit after each phase
git commit -m "refactor: <phase description>"
```

### Continuous Integration

After each phase:
```bash
# Compile
cmake --build build --target engine

# Run tests
./build/test/unit/engine/render_system_test

# Run engine
./build/bin/editor
```

---

## Questions to Consider

Before starting:
1. Do we have good test coverage of current behavior?
2. Are there pending features that might conflict?
3. Is this the right time (not close to deadline)?
4. Who will review the refactoring?

During refactoring:
1. Is each class still single-responsibility?
2. Are dependencies explicit and minimal?
3. Can this class be easily unit tested?
4. Is the API intuitive?

---

## Next Steps

1. **Review this plan** with team
2. **Get approval** for time allocation
3. **Create backup branch**
4. **Start Phase 0** (preparation)
5. **Execute incrementally**

**Remember:** The system stays functional after each phase. You can pause and ship at any point.
