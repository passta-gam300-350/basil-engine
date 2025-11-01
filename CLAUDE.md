# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

GAM300 is a modern C++ game engine with a focus on graphics rendering. The project uses CMake as the build system and targets C++20 with Visual Studio 2022 on Windows. The engine features a standalone graphics library with modern OpenGL 4.6 support, PBR rendering, and ECS integration capabilities.

## Build System & Dependencies

The project uses CMake 3.20+ with custom dependency management through FetchContent. All external dependencies are automatically downloaded and built.

### Build Commands

```bash
# Generate Visual Studio solution
cmake -G "Visual Studio 17 2022" -B build

# Open generated solution
start "" "build/gam300.sln"

# Clean build artifacts
rm -rf build/
```

### Visual Studio CMake Integration
You can also open the root directory directly in Visual Studio 2022, which has built-in CMake support.

## Architecture

### High-Level Structure

```
gam300/
├── editor/           # ImGui-based level editor (main thread)
├── engine/           # ECS-based game engine (engine thread)
├── lib/
│   ├── Graphics/     # Standalone OpenGL 4.6 rendering library
│   ├── JobSystem/    # Coroutine-based job system (C++20)
│   ├── Scripting/    # Mono C# scripting integration
│   ├── resource/     # Asset loading and management
│   └── common/       # Lock-free data structures
├── bin/              # Runtime output (executables + assets)
├── test/             # Examples and unit tests
└── cmake/            # Build system utilities
```

### Graphics Library Architecture

The graphics library (`lib/Graphics/`) is designed as a standalone, framework-agnostic rendering system:

- **Pipeline-Based Rendering**: Command buffer architecture with render passes
- **Data Submission API**: Applications push rendering data each frame (no ECS coupling)
- **Modern OpenGL 4.6**: Bindless textures, compute shaders, and advanced features
- **PBR Lighting**: Physically-based rendering with material system
- **Efficient Rendering**: Built-in frustum culling, instancing, and shadow mapping

Key components:
- `SceneRenderer`: Main entry point for rendering operations
- `RenderPipeline`: Configurable rendering passes (shadow, main, post-process)
- `ResourceManager`: Asset loading and management
- `Material` & `Shader`: Modern shader and material system with property binding

### Multi-Threaded Architecture

**CRITICAL**: The engine uses a dual-context, multi-threaded rendering architecture:

#### Thread Model

```
MAIN THREAD (Editor)              ENGINE THREAD (Game Loop)
├─ GLFW window (visible)          ├─ GLFW window (hidden, GLFW_VISIBLE=false)
├─ OpenGL Context #1              ├─ OpenGL Context #2 (SHARED with Context #1)
├─ ImGui rendering                ├─ ECS systems (physics, logic, etc.)
├─ Editor UI                      ├─ RenderSystem::Update()
├─ Event polling                  ├─ SceneRenderer->Render() (PBR pipeline)
└─ Presents to screen             └─ Renders to FBO textures
```

#### OpenGL Context Sharing

Both contexts are **active simultaneously** on different threads:
- **Context #1 (Main)**: Created in `editor/main.cpp`, used for ImGui
- **Context #2 (Engine)**: Created in `lib/Graphics/Window.cpp:98` with share parameter
- **Shared Resources**: Textures, VBOs, shaders (GPU memory is shared)
- **Not Shared**: VAO bindings, framebuffer bindings, GL state

#### Synchronization Mechanism

```cpp
// Engine thread blocks waiting for editor to consume frame
m_container_is_presentable.acquire();  // EngineService.cpp:15

// Main thread releases semaphore after reading texture
engineService.m_cont->m_container_is_presentable.release();  // EditorMain.cpp:165
```

**Data Flow**:
1. Engine renders to `editorResolvedBuffer` FBO (Context #2)
2. Engine blocks on semaphore
3. Editor reads texture ID from shared FrameData struct
4. Editor displays texture via `ImGui::Image()` (Context #1)
5. Editor releases semaphore, engine continues

**Note**: Uses CPU-level sync (semaphore), not GPU-level sync (no `glFenceSync`). Works due to implicit driver sync and frame pacing.

### ECS Architecture

The engine uses a custom ECS wrapper around EnTT:

- **World**: Top-level container (`ecs::world`)
- **Entity**: Handle with UUID support (`ecs::entity`)
- **Components**: Pure data structs (MeshRendererComponent, TransformComponent, etc.)
- **Systems**: Logic processors (RenderSystem, PhysicsSystem, TransformSystem)
- **Scheduler**: Dependency graph-based parallel execution using JobSystem

**Key Systems**:
- `RenderSystem`: Queries entities, submits to Graphics lib (no longer singleton after Phase 5 refactor)
- `PhysicsSystem`: Jolt Physics integration
- `TransformSystem`: Hierarchical transform updates
- `BehaviourSystem`: C# script execution via Mono

### Job System

Coroutine-based work-stealing job scheduler (`lib/JobSystem/`):

```cpp
JobSystem m_JobSystem{4};  // 4 worker threads
JobID job = m_JobSystem.submit(preferredWorker, dependencies, []() -> JobTask {
    // Work here
    co_return;
});
m_JobSystem.wait_for(job);
```

- **Lock-free**: SPMC deques for work stealing
- **Dependency graph**: Jobs can depend on other jobs
- **Coroutine-based**: Uses C++20 coroutines for task suspension/resumption

### Core Dependencies

- **OpenGL 4.6**: Graphics API (via GLAD)
- **GLFW**: Window and input management
- **GLM**: Mathematics library
- **Assimp**: 3D model loading
- **EnTT**: Entity-Component-System (v3.15.0)
- **ImGui**: Immediate mode GUI (docking branch)
- **spdlog**: Logging library
- **stb**: Image loading utilities
- **Jolt Physics**: 3D physics engine
- **Mono**: C# scripting runtime
- **yaml-cpp**: Configuration file parsing

## Development Workflow

### Graphics Development

The graphics library supports multiple integration patterns:

1. **Direct Usage**: Submit renderable data directly to `SceneRenderer`
2. **ECS Integration**: Use graphics components with any ECS library
3. **Custom Data Structures**: Works with arrays, custom containers, etc.

Example graphics test is available at `test/examples/lib/Graphics/main.cpp`.

### Testing

- **Examples**: Located in `test/examples/` for integration testing
- **Unit Tests**: Placeholder structure in `test/unit/`
- **Graphics Test**: Runnable example with assets in `test/examples/lib/Graphics/`

### Asset Pipeline

The graphics library includes:
- Model loading via Assimp (.obj, .fbx, etc.)
- Texture loading via stb_image (PNG, JPG, etc.)
- Shader compilation and management
- Material serialization system (planned)

### Key Files to Understand

- `lib/Graphics/include/Scene/SceneRenderer.h`: Main graphics API
- `lib/Graphics/STANDALONE_USAGE_EXAMPLE.md`: Integration examples
- `lib/Graphics/EDITOR_INTEGRATION_ROADMAP.md`: Future development plans
- `test/examples/lib/Graphics/main.cpp`: Working graphics example

## Development Notes

### Critical Implementation Details

1. **OpenGL Threading Rules**:
   - Each thread has its own context (no context switching during runtime)
   - Contexts share GPU resources (textures, buffers, shaders)
   - `Window.cpp:97` has a commented-out `glfwMakeContextCurrent(ptr)` - this line was unnecessary

2. **RenderSystem Refactor (Phase 5)**:
   - No longer a singleton - now owned by Engine instance
   - Access via `Engine::GetRenderSystem()` instead of static methods
   - Modular design: ShaderLibrary, PrimitiveManager, RenderResourceCache, ComponentInitializer

3. **Material System (3-Tier)**:
   - `Material`: Shared base material
   - `MaterialInstance`: Full copy-on-write (breaks instancing)
   - `MaterialPropertyBlock`: Lightweight per-object overrides (preserves instancing)

4. **Scene Management**:
   - Hierarchical scenes with subscene streaming
   - Proximity-based loading for large open worlds
   - GUID-based asset references

5. **Rendering Pipeline**:
   - Shadow mapping (directional/point/spot) → Main PBR → Bloom → HDR → Tone mapping → Editor resolve
   - SSBO-based instancing (96 bytes per instance)
   - Supports 50+ shadow-casting lights via unified shadow system
   - Adaptive PCF shadow sampling with Poisson disk distribution

### Editor Architecture

The editor (`editor/`) is organized into screens and services:

- **Screens**: `EditorMain`, `SplashScreen`, `ProjectMenu`
- **Services**: `EngineService` (manages engine thread), `FileService` (project management)
- **Asset Management**: `AssetManager` for editor-side resource handling

**EngineService** is the middleman between Editor and Engine:
- Spawns engine thread via `std::jthread`
- Manages snapshot synchronization for entity/component data
- Provides thread-safe communication via queues and semaphores

### Shader Organization

Shaders are in `bin/assets/shaders/`:
- `main_pbr.vert/frag`: Primary PBR rendering with Cook-Torrance BRDF
- `shadow_depth.vert/frag`: Depth-only rendering for shadow maps
- `hdr_luminance.comp`: Compute shader for HDR luminance calculation
- `tone_map.vert/frag`: Tone mapping (ACES, Reinhard, etc.)
- `bloom.frag`: Gaussian blur for bloom effect

### Resource System

GUID-based async resource loading (`engine/include/Manager/ResourceSystem.hpp`):
- Job-based async loading with dependencies
- Reference counting for memory management
- Hot-reload support (planned)
- Type-safe resource handles

### Known Issues & Gotchas

1. **Window.cpp Context Switch**: Line 97 creates temporary context ownership violation (commented out, safe to remove)
2. **GPU Synchronization**: No explicit `glFenceSync` - relies on implicit driver sync
3. **MaterialPropertyBlock**: Implemented but not yet used in instanced rendering pipeline
4. **Shader Hot-Reload**: Planned but not implemented

### Future Roadmap

- Material serialization and editor
- Shader hot-reload system
- Enhanced ECS integration for Graphics lib
- GPU-level synchronization with `glFenceSync`
- LOD system for meshes
- Compute shader integration for particles

## Important Files

- `editor/src/Service/EngineService.cpp`: Multi-threading implementation
- `engine/src/Render/Render.cpp`: RenderSystem implementation (post-refactor)
- `lib/Graphics/src/Scene/SceneRenderer.cpp`: Main rendering orchestration
- `lib/Graphics/src/Core/Window.cpp`: OpenGL context creation (dual-context)
- `lib/JobSystem/include/jobsystem.hpp`: Coroutine job system
- `engine/include/Ecs/system/scheduler.h`: ECS job scheduling

## Build Artifacts

- Runtime output directory is `bin/` at project root level
- Executables: `bin/editor.exe`, `bin/engine.exe`
- Assets: `bin/assets/` (shaders, textures, models)
- Configuration: `Default.yaml` in working directory