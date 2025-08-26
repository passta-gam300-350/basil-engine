# Graphics Architecture Documentation

## Table of Contents
1. [Architecture Overview](#architecture-overview)
2. [Core Systems](#core-systems)
3. [File Structure & Responsibilities](#file-structure--responsibilities)
4. [Workflow & Data Flow](#workflow--data-flow)
5. [ECS Integration](#ecs-integration)
6. [Current Features](#current-features)
7. [Implementation Guide for Advanced Features](#implementation-guide-for-advanced-features)
8. [Suggested Improvements](#suggested-improvements)
9. [Known Issues](#known-issues)

---

## Architecture Overview

The graphics library follows a **modern, clean architecture** with clear separation between Engine-level ECS systems and Graphics-level rendering coordination. This design ensures maintainability, performance, and extensibility.

### Key Architectural Principles
- **Value-Based Commands**: Efficient command system using `std::variant` for cache-friendly execution
- **Component Separation**: Graphics library doesn't directly depend on Engine components
- **Resource Decoupling**: CPU resource loading separated from GPU resource creation
- **Bindless Ready**: Abstracted texture binding system for future bindless support
- **Sorting Optimization**: Commands are sortable for optimal GPU state changes

### High-Level Architecture Diagram
```
┌─────────────────────────────────────────────────────────────────┐
│                        APPLICATION LAYER                        │
├─────────────────────────────────────────────────────────────────┤
│  ENGINE LAYER           │        GRAPHICS LAYER                 │
│                         │                                       │
│  ┌─────────────────┐   │  ┌─────────────────────────────────┐  │
│  │ ECS Systems     │   │  │ Rendering Coordinators          │  │
│  │ - Transform     │◄──┼──┤ - MeshRenderer                  │  │
│  │ - Visibility    │   │  │ - FrustumCuller                 │  │
│  │ - Physics       │   │  │ - LightingSystem (future)       │  │
│  └─────────────────┘   │  └─────────────────────────────────┘  │
│                         │                                       │
│  ┌─────────────────┐   │  ┌─────────────────────────────────┐  │
│  │ Scene & Entity  │   │  │ Command System                  │  │
│  │ Management      │   │  │ - Value-based commands          │  │
│  └─────────────────┘   │  │ - Efficient sorting             │  │
│                         │  │ - Batch execution               │  │
└─────────────────────────┼─────────────────────────────────────┘
                          │
                          │  ┌─────────────────────────────────┐
                          │  │ Resource Management             │
                          │  │ - Decoupled loading/GPU         │
                          │  │ - Texture binding abstraction  │
                          │  └─────────────────────────────────┘
```

---

## Core Systems

### 1. Command System (`Core/RenderCommandBuffer.h/.cpp`)
The heart of the graphics system, providing efficient, sortable rendering commands.

**Key Components:**
- **Value-Based Commands**: Uses `std::variant<>` instead of virtual functions
- **Sort Keys**: 64-bit keys for optimal command ordering
- **Batch Execution**: Efficient command processing with `std::visit`

```cpp
// Command types
struct BindShaderData { std::shared_ptr<Shader> shader; };
struct SetUniformsData { /* matrices, camera data */ };
struct BindTexturesData { std::vector<Texture> textures; };
struct DrawElementsData { uint32_t vao, indexCount; };

// Sort key for batching
struct CommandSortKey {
    uint8_t pass;          // Rendering pass (0-255)
    uint32_t material;     // Material ID for batching
    uint16_t mesh;         // Mesh ID
    uint16_t instance;     // Instance ID
};
```

### 2. Renderer (`Core/Renderer.h/.cpp`)
Central rendering coordinator managing the command buffer and graphics context.

**Responsibilities:**
- Frame lifecycle management (BeginFrame/EndFrame)
- Command submission and sorting
- Graphics context coordination

### 3. Resource Management (`Resources/`)
Decoupled resource system separating CPU loading from GPU creation.

**Key Files:**
- `Texture.h/.cpp`: CPU/GPU separated texture loading
- `TextureBindingSystem.h/.cpp`: Bindless-ready texture binding
- `Shader.h/.cpp`, `Material.h/.cpp`, `Mesh.h/.cpp`: Core resources

### 4. Scene Rendering (`Scene/SceneRenderer.h/.cpp`)
High-level coordinator that orchestrates rendering operations.

**Workflow:**
1. Manages rendering coordinators (MeshRenderer, FrustumCuller)
2. Executes render pipeline
3. Coordinates between ECS and Graphics systems

---

## File Structure & Responsibilities

### Core Graphics (`lib/Graphics/`)

```
lib/Graphics/
├── include/
│   ├── Core/
│   │   ├── Renderer.h                    # Central rendering coordinator
│   │   ├── RenderCommandBuffer.h         # Modern command system
│   │   ├── RenderCommand.h               # Legacy compatibility layer
│   │   ├── GraphicsContext.h             # OpenGL context management
│   │   └── Window.h                      # Window management
│   │
│   ├── Resources/
│   │   ├── Texture.h                     # Decoupled texture loading
│   │   ├── TextureBindingSystem.h        # Bindless-ready binding
│   │   ├── Shader.h                      # Shader management
│   │   ├── Material.h                    # Material system
│   │   ├── Mesh.h                        # Geometry data
│   │   └── ResourceManager.h             # Resource coordination
│   │
│   ├── ECS/
│   │   ├── ComponentInterfaces.h         # Engine/Graphics decoupling
│   │   └── Components/                   # Graphics-specific components
│   │       ├── MeshComponent.h
│   │       └── MaterialComponent.h
│   │
│   ├── Rendering/
│   │   ├── MeshRenderer.h                # Mesh rendering coordinator
│   │   └── FrustumCuller.h               # Camera frustum culling
│   │
│   ├── Scene/
│   │   └── SceneRenderer.h               # High-level scene coordination
│   │
│   └── Utility/
│       ├── Camera.h                      # Camera mathematics
│       └── Light.h                       # Lighting data structures
│
└── src/ [corresponding .cpp implementations]
```

### Engine Layer (`test/examples/lib/Graphics/Engine/`)

```
Engine/
├── ECS/
│   ├── Components/
│   │   ├── TransformComponent.h          # Transform mathematics
│   │   └── VisibilityComponent.h         # Visibility state
│   │
│   ├── Systems/
│   │   ├── TransformSystem.h/.cpp        # Pure ECS transform logic
│   │   └── VisibilitySystem.h/.cpp       # Pure ECS visibility logic
│   │
│   └── ComponentAccessor.h/.cpp          # Graphics/Engine interface
│
└── Scene/
    ├── Scene.h/.cpp                      # ECS registry management
    └── Entity.h/.cpp                     # Entity creation/management
```

---

## Workflow & Data Flow

### 1. Initialization Sequence
```
Application::InitializeGraphicsEngine()
├── 1. Window (OpenGL context)
├── 2. Renderer (command system)
├── 3. ResourceManager (asset loading)
├── 4. Scene (ECS registry)
├── 4.5. ComponentAccessor (Engine/Graphics bridge)
├── 5. Camera (view/projection)
└── 6. SceneRenderer (rendering coordination)
```

### 2. Frame Rendering Workflow
```
Application::RenderFrame()
├── Renderer::BeginFrame()
│   ├── Clear command buffer
│   └── Clear screen
│
├── SceneRenderer::Render()
│   ├── RenderPipeline::Execute()
│   │   ├── FrustumCuller::CullAgainstCamera()
│   │   │   ├── Query entities with MeshComponent
│   │   │   ├── Get Transform/Visibility via ComponentAccessor
│   │   │   └── Update visibility based on camera frustum
│   │   │
│   │   └── MeshRenderer::Render()
│   │       ├── Query visible entities
│   │       ├── For each visible mesh:
│   │       │   ├── Generate sort key (pass/material/mesh/instance)
│   │       │   ├── Submit BindShaderData command
│   │       │   ├── Submit SetUniformsData command
│   │       │   ├── Submit BindTexturesData command
│   │       │   └── Submit DrawElementsData command
│   │       └── Commands queued for sorting
│   │
│   └── Commands submitted to RenderCommandBuffer
│
└── Renderer::EndFrame()
    ├── RenderCommandBuffer::Sort() (by sort keys)
    ├── RenderCommandBuffer::Execute() (batch processing)
    └── SwapBuffers()
```

### 3. Command Processing Pipeline
```
Command Generation → Sort Key Creation → Command Submission
                                              ↓
Command Sorting → Batch Execution → GPU State Changes
     ↓                 ↓                    ↓
(by material,    (std::visit        (Minimized driver
 pass, mesh)     dispatch)           overhead)
```

---

## ECS Integration

### Component Accessor Pattern
The graphics library uses an interface-based system to access Engine components without direct dependencies.

```cpp
// Graphics library interface
class ITransformComponent {
public:
    virtual glm::mat4 GetTransform() const = 0;
    virtual glm::vec3 GetPosition() const = 0;
};

// Engine implementation
class TransformComponentAdapter : public ITransformComponent {
    TransformComponent& m_Transform;
public:
    glm::mat4 GetTransform() const override { 
        return m_Transform.GetTransform(); 
    }
};
```

### Data Flow Between Layers
```
ECS Systems (Engine) → ComponentAccessor → Rendering Coordinators (Graphics)
        ↓                      ↓                       ↓
   Game Logic            Type Erasure              Render Commands
   Physics               Interface              Command Buffer
   Transform                                   GPU Execution
```

### Component Responsibilities

**Engine Components:**
- `TransformComponent`: Position, rotation, scale, matrix calculations
- `VisibilityComponent`: Visibility flags, culling state, bounding data

**Graphics Components:**
- `MeshComponent`: Mesh resource reference, VAO handle
- `MaterialComponent`: Material resource references, shader bindings

---

## Current Features

### ✅ Implemented Features

1. **Modern Command System**
   - Value-based commands using `std::variant`
   - Efficient sorting by material, pass, and mesh
   - Batch command execution with `std::visit`

2. **Resource Management**
   - CPU/GPU decoupled texture loading
   - Move semantics for efficient resource transfers
   - Resource caching and management

3. **Texture Binding Abstraction**
   - Traditional OpenGL binding implementation
   - Bindless texture interface (stub ready)
   - Factory pattern for easy switching

4. **ECS Integration**
   - Clean separation between Engine and Graphics
   - Component accessor pattern for decoupling
   - Pure ECS systems vs. rendering coordinators

5. **Rendering Pipeline**
   - Multi-pass rendering support
   - Framebuffer management
   - Camera frustum culling

6. **Asset Loading**
   - Model loading via Assimp
   - Shader compilation and management
   - Texture loading with STB

### 🚧 Partially Implemented

1. **Lighting System**
   - Basic structure exists
   - Needs integration with command system

2. **Material System**
   - Basic material support
   - Needs PBR workflow implementation

---

## Implementation Guide for Advanced Features

### 1. BVH (Bounding Volume Hierarchy) for Culling

**Where to implement:** `lib/Graphics/include/Rendering/BVHCuller.h`

```cpp
class BVHCuller {
public:
    struct BVHNode {
        AABB bounds;
        std::vector<entt::entity> entities;  // Leaf nodes only
        std::unique_ptr<BVHNode> left, right;
    };
    
    void BuildBVH(entt::registry& registry);
    void CullAgainstFrustum(const Frustum& frustum, std::vector<entt::entity>& visibleEntities);
    
private:
    std::unique_ptr<BVHNode> m_Root;
    void BuildRecursive(std::vector<entt::entity>& entities, int depth);
};
```

**Integration Points:**
1. Replace `FrustumCuller` in `SceneRenderer::InitializeRenderingCoordinators()`
2. Build BVH when scene changes significantly
3. Use spatial partitioning for large numbers of objects

**Performance Considerations:**
- Rebuild BVH when >10% of objects move
- Use incremental updates for dynamic objects
- Consider GPU-based BVH for very large scenes

### 2. SSBO for Bindless Textures

**Where to implement:** `lib/Graphics/src/Resources/TextureBindingSystem.cpp`

```cpp
class BindlessTextureBinding : public ITextureBindingSystem {
private:
    struct TextureHandle {
        GLuint64 handle;
        GLuint textureId;
    };
    
    GLuint m_HandleSSBO;
    std::vector<TextureHandle> m_TextureHandles;
    std::unordered_map<GLuint, size_t> m_TextureToIndex;
    
public:
    void InitializeSSBO(size_t maxTextures = 1024);
    void BindTextures(const std::vector<Texture>& textures, std::shared_ptr<Shader> shader) override;
};
```

**Implementation Steps:**
1. Check for `GL_ARB_bindless_texture` extension
2. Create SSBO for texture handles: `glGenBuffers(1, &m_HandleSSBO)`
3. Generate texture handles: `glGetTextureHandleARB(textureId)`
4. Make handles resident: `glMakeTextureHandleResidentARB(handle)`
5. Update SSBO with handle array
6. Bind SSBO to shader: `glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_HandleSSBO)`

**Shader Integration:**
```glsl
layout(std430, binding = 0) readonly buffer TextureHandles {
    uint64_t handles[];
};

uniform int textureIndices[MAX_TEXTURES];
// Use: texture(sampler2D(handles[textureIndices[0]]), texCoord)
```

### 3. GPU Instancing

**Where to implement:** `lib/Graphics/include/Rendering/InstanceRenderer.h`

```cpp
class InstanceRenderer {
public:
    struct InstanceData {
        glm::mat4 modelMatrix;
        glm::vec4 color;
        uint32_t materialId;
    };
    
    void AddInstance(const InstanceData& instance);
    void Render(const std::string& meshName, Camera& camera);
    void Clear();
    
private:
    std::vector<InstanceData> m_InstanceData;
    GLuint m_InstanceVBO;
    void UpdateInstanceBuffer();
};
```

**Command System Integration:**
```cpp
struct DrawInstancedData {
    uint32_t vao;
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t instanceVBO;
};
```

**Implementation Steps:**
1. Create instance VBO with `GL_DYNAMIC_DRAW`
2. Update instance data each frame: `glBufferSubData`
3. Bind instance VBO as vertex buffer with divisor: `glVertexAttribDivisor(location, 1)`
4. Use `glDrawElementsInstanced` instead of `glDrawElements`

### 4. Deferred Rendering Pipeline

**Where to implement:** `lib/Graphics/include/Pipeline/DeferredPipeline.h`

```cpp
class DeferredPipeline : public RenderPipeline {
public:
    void Initialize(uint32_t width, uint32_t height) override;
    
private:
    // G-Buffer pass
    std::shared_ptr<RenderPass> m_GeometryPass;
    std::unique_ptr<FrameBuffer> m_GBuffer;
    
    // Lighting pass
    std::shared_ptr<RenderPass> m_LightingPass;
    std::unique_ptr<Shader> m_LightingShader;
    
    // Forward pass (for transparent objects)
    std::shared_ptr<RenderPass> m_ForwardPass;
};
```

**G-Buffer Layout:**
- RT0: Albedo (RGB) + Metallic (A)
- RT1: Normal (RGB) + Roughness (A)  
- RT2: Position (RGB) + AO (A)
- RT3: Entity ID (R) + Motion Vector (GB)
- Depth: Depth + Stencil

### 5. Compute Shader Integration

**Where to implement:** `lib/Graphics/include/Compute/ComputeDispatcher.h`

```cpp
class ComputeDispatcher {
public:
    void Dispatch(std::shared_ptr<ComputeShader> shader, 
                  uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ);
    void DispatchIndirect(GLuint indirectBuffer);
    
    // For GPU-driven rendering
    void CullOnGPU(const CameraData& camera, GLuint instanceSSBO);
};
```

**Use Cases:**
- GPU-driven culling and LOD selection
- Particle system updates
- Post-processing effects
- Mesh optimization (decimation, normal generation)

---

## Suggested Improvements

### High Priority

1. **GPU Resource Pooling**
   - Implement buffer/texture pools to reduce allocation overhead
   - Add automatic resource defragmentation
   - Location: `lib/Graphics/include/Resources/ResourcePool.h`

2. **Multi-threaded Command Generation**
   - Thread-safe command buffer per thread
   - Merge command buffers before sorting
   - Location: `lib/Graphics/include/Core/ThreadedCommandBuffer.h`

3. **Shader Reflection System**
   - Automatic uniform binding based on reflection
   - Validation of shader/material compatibility
   - Location: `lib/Graphics/include/Resources/ShaderReflection.h`

### Medium Priority

4. **Asset Streaming System**
   - LOD-based model loading
   - Texture streaming for large worlds
   - Location: `lib/Graphics/include/Streaming/AssetStreamer.h`

5. **Debug Renderer**
   - Wireframe, bounding box, normal visualization
   - Performance profiling overlays
   - Location: `lib/Graphics/include/Debug/DebugRenderer.h`

6. **Material Graph System**
   - Node-based material editor data structures
   - Shader generation from graphs
   - Location: `lib/Graphics/include/Materials/MaterialGraph.h`

### Low Priority

7. **Temporal Anti-Aliasing (TAA)**
   - Motion vector generation
   - Temporal accumulation
   - Location: `lib/Graphics/include/PostProcess/TAA.h`

8. **Screen Space Reflections**
   - Ray marching in screen space
   - Hierarchical depth buffer
   - Location: `lib/Graphics/include/PostProcess/SSR.h`

---

## Known Issues

### Critical Issues
1. **Cross-Library Dependencies**
   - Status: ✅ **RESOLVED** - Component accessor pattern implemented
   - Impact: Clean separation between Engine and Graphics

2. **Command Sorting Performance**
   - Status: ✅ **RESOLVED** - Value-based commands with efficient sort keys
   - Impact: Optimal GPU state changes

### Minor Issues

3. **Resource Loading Blocking**
   - Issue: Asset loading blocks main thread
   - Impact: Frame drops during loading
   - Solution: Implement background loading with `std::async`

4. **Memory Fragmentation**
   - Issue: No buffer/texture pooling
   - Impact: Allocation overhead in dynamic scenes
   - Solution: Implement resource pools

5. **Limited Error Handling**
   - Issue: OpenGL errors not consistently checked
   - Impact: Silent failures in release builds
   - Solution: Add debug callback and error checking macros

### Future Considerations

6. **Platform Abstraction**
   - Current: OpenGL only
   - Future: Add Vulkan/DirectX abstraction layer
   - Location: `lib/Graphics/include/Platform/RenderAPI.h`

7. **Asset Format Optimization**
   - Current: Assimp loading (slow)
   - Future: Custom binary format for faster loading
   - Location: `lib/Graphics/include/Assets/BinaryAssetLoader.h`

---

## Performance Guidelines

### Command System Best Practices
1. **Minimize State Changes**: Sort by material, then mesh
2. **Batch Similar Objects**: Use instancing for identical meshes
3. **Reduce Command Count**: Combine operations where possible

### Resource Management
1. **Preload Resources**: Load during initialization, not runtime
2. **Use Resource Pools**: Reuse buffers/textures to reduce allocations
3. **Texture Atlasing**: Combine small textures to reduce bind count

### Memory Management
1. **Move Semantics**: Use `std::move` for large resource transfers
2. **Smart Pointers**: Use `shared_ptr` for shared resources
3. **RAII**: Ensure proper cleanup of OpenGL resources

---

## Development Workflow

### Adding New Features
1. **Design Phase**: Define interfaces and data structures
2. **Implementation**: Start with CPU implementation
3. **Testing**: Unit tests for core functionality
4. **Integration**: Add to command system and pipeline
5. **Optimization**: Profile and optimize performance
6. **Documentation**: Update this architecture guide

### Debugging Graphics Issues
1. **Use RenderDoc**: Frame debugging and GPU profiling
2. **OpenGL Debug Context**: Enable debug callbacks
3. **Command Buffer Inspection**: Log command submission
4. **Resource Tracking**: Monitor GPU memory usage

### Contributing Guidelines
1. **Follow Architecture**: Maintain separation of concerns
2. **Value Semantics**: Prefer value types over pointers
3. **Interface Design**: Use abstract interfaces for extensibility
4. **Performance First**: Consider cache-friendliness and GPU efficiency

---

*This document should be updated whenever significant architectural changes are made to the graphics system.*