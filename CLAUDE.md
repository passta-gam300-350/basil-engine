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
├── lib/Graphics/     # Standalone graphics library (main component)
├── engine/           # Main engine code (currently minimal)
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

### Core Dependencies

- **OpenGL 4.6**: Graphics API (via GLAD)
- **GLFW**: Window and input management
- **GLM**: Mathematics library
- **Assimp**: 3D model loading
- **EnTT**: Entity-Component-System (v3.15.0)
- **ImGui**: Immediate mode GUI (docking branch)
- **spdlog**: Logging library
- **stb**: Image loading utilities

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
- `engine/RENDER_SYSTEM_ARCHITECTURE.md`: **NEW** - Detailed RenderSystem architecture documentation
- `engine/RENDER_SYSTEM_REFACTOR_EXECUTION_PLAN.md`: Refactoring plan (Phases 0-6)

## RenderSystem Architecture (2025 Refactoring)

The RenderSystem has been refactored from a monolithic singleton into a clean, modular architecture:

### Before Refactoring
- Single monolithic class (~420 lines)
- Static singleton pattern with hidden dependencies
- 6 file-scope global variables (mutable state)
- Mixed responsibilities (rendering + shader loading + primitive generation + initialization)

### After Refactoring (Phases 1-6 Complete)
- **RenderSystem**: Main coordinator (owned by Engine, NOT a singleton)
- **ShaderLibrary**: Shader loading and caching
- **PrimitiveManager**: Primitive mesh generation (cube, plane, etc.)
- **RenderResourceCache**: Entity-specific resource caching
- **ComponentInitializer**: ECS component initialization logic

**Key Changes:**
- ✅ No singleton pattern - access via `Engine::GetRenderSystem()`
- ✅ No file-scope variables - explicit ownership
- ✅ Clear separation of concerns
- ✅ Fully documented with Doxygen comments
- ✅ Each class < 300 lines

**Usage:**
```cpp
// OLD (before refactoring)
RenderSystem::Instance().m_SceneRenderer->Render();

// NEW (after refactoring)
Engine::GetRenderSystem().m_SceneRenderer->Render();
```

See `engine/RENDER_SYSTEM_ARCHITECTURE.md` for complete architecture details, data flow diagrams, and usage examples.

## Development Notes

- The engine is currently on the `render-system-refactor` branch with completed RenderSystem refactoring (Phases 1-6)
- Graphics library is designed to be completely standalone and framework-agnostic
- Future roadmap includes material serialization, hot-reload, and enhanced ECS integration
- Runtime output directory is configured to `bin/` at project root level