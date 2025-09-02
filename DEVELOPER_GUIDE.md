# Developer Guide: Extending the Graphics Framework

A comprehensive guide for developers who want to extend, modify, or contribute to the graphics framework.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Framework Architecture Deep Dive](#framework-architecture-deep-dive)
3. [Adding New Features](#adding-new-features)
4. [Performance Optimization Guide](#performance-optimization-guide)
5. [Debugging and Profiling](#debugging-and-profiling)
6. [Best Practices](#best-practices)
7. [Common Patterns](#common-patterns)
8. [Troubleshooting](#troubleshooting)

---

## Getting Started

### Prerequisites for Development

**Required Knowledge:**
- Modern C++17/20 (smart pointers, move semantics, templates, `std::variant`)
- OpenGL 4.5+ (shaders, buffers, state management)
- Linear algebra (matrices, vectors, transformations)
- Basic ECS concepts (entities, components, systems)

**Tools You'll Need:**
- Visual Studio 2022 or equivalent C++20 compiler
- Graphics debuggers: RenderDoc, NVIDIA Nsight Graphics, or AMD GPU PerfStudio
- Git for version control
- CMake 3.20+ for build configuration

### Development Setup

```bash
# Clone and setup development environment
git clone <repository-url>
cd gam300

# Create development build
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DGRAPHICS_DEBUG=ON
cmake --build build --config Debug

# Run with debug output
cd bin
./GraphicsTest.exe --debug --verbose
```

### Code Organization Philosophy

The framework follows these architectural principles:

**1. Value Semantics Over Pointer Chasing**
```cpp
// Preferred: Value-based commands
struct DrawCommand {
    uint32_t vao;
    uint32_t indexCount;
    // No pointers, easily copyable, cache-friendly
};

// Avoid: Pointer-heavy virtual dispatch
class IDrawCommand {
    virtual void Execute() = 0;  // Virtual calls, heap allocation
};
```

**2. Data-Oriented Design**
```cpp
// Preferred: Structure of Arrays
struct InstanceData {
    std::vector<glm::mat4> transforms;    // All transforms together
    std::vector<glm::vec4> colors;        // All colors together
    std::vector<uint32_t> materialIds;    // All material IDs together
};

// Avoid: Array of Structures (poor cache usage)
struct Instance {
    glm::mat4 transform;  // Mixed data layout
    glm::vec4 color;
    uint32_t materialId;
};
std::vector<Instance> instances;
```

**3. Composition Over Inheritance**
```cpp
// Preferred: Component composition
class SceneRenderer {
    std::unique_ptr<MeshRenderer> m_MeshRenderer;      // Composable
    std::unique_ptr<InstancedRenderer> m_InstancedRenderer;
    std::unique_ptr<FrustumCuller> m_FrustumCuller;
};

// Avoid: Deep inheritance hierarchies
class BaseRenderer {
    virtual void Render() = 0;
};
class MeshRenderer : public BaseRenderer { /* ... */ };
class InstancedRenderer : public BaseRenderer { /* ... */ };
```

---

## Framework Architecture Deep Dive

### The Command System: Heart of Performance

Understanding the command system is crucial for any extension work:

```cpp
// lib/Graphics/include/Core/RenderCommandBuffer.h

// The command buffer is a vector of sortable commands
class RenderCommandBuffer {
private:
    struct SortableCommand {
        VariantRenderCommand command;        // The actual command (std::variant)
        RenderCommands::CommandSortKey key;  // 64-bit sort key
        
        // Comparison operator for sorting
        bool operator<(const SortableCommand& other) const {
            return key.GetSortKey() < other.key.GetSortKey();
        }
    };
    
    std::vector<SortableCommand> m_Commands;  // Main command storage
    std::unique_ptr<ITextureBindingSystem> m_TextureBindingSystem;
};
```

**Why This Design is Fast:**

1. **Cache-Friendly Storage**: Commands are stored in contiguous memory
2. **Zero Virtual Calls**: `std::variant` uses compile-time dispatch
3. **Optimal Sorting**: 64-bit integer comparison is extremely fast
4. **Batch Processing**: Similar commands execute together

### Sort Key Engineering

The 64-bit sort key is carefully designed for optimal GPU performance:

```cpp
struct CommandSortKey {
    uint8_t  pass;      // [63-56] Higher = later (shadows before opaque before transparent)
    uint32_t material;  // [55-24] Group by shader/material to minimize state changes
    uint16_t mesh;      // [23-8]  Group by geometry (VAO binding)
    uint16_t instance;  // [7-0]   Render order within same mesh/material
    
    uint64_t GetSortKey() const {
        return ((uint64_t)pass << 56) |
               ((uint64_t)material << 24) |
               ((uint64_t)mesh << 8) |
               (uint64_t)instance;
    }
};
```

**Sort Priority Reasoning:**
1. **Pass** (Most Important): Different rendering algorithms (shadow depth vs color)
2. **Material**: Most expensive state change (shader programs, textures)
3. **Mesh**: Moderate cost (VAO binding, vertex format changes) 
4. **Instance**: Cheapest (uniform updates only)

### ECS Abstraction Layer

The framework achieves clean separation between Engine and Graphics through abstract interfaces:

```cpp
// Graphics library defines interfaces (no Engine dependencies)
class ITransformComponent {
public:
    virtual glm::mat4 GetTransform() const = 0;
    virtual glm::vec3 GetPosition() const = 0;
    virtual glm::vec3 GetRotation() const = 0;
    virtual glm::vec3 GetScale() const = 0;
    virtual void SetPosition(const glm::vec3& pos) = 0;
    virtual void SetRotation(const glm::vec3& rot) = 0;
    virtual void SetScale(const glm::vec3& scale) = 0;
};

class IComponentAccessor {
public:
    virtual ITransformComponent* GetTransform(entt::registry& registry, 
                                              entt::entity entity) = 0;
    virtual IVisibilityComponent* GetVisibility(entt::registry& registry, 
                                                 entt::entity entity) = 0;
    // Add new component types here as needed
};
```

```cpp
// Engine implements the interfaces (knows about concrete types)
class EngineComponentAccessor : public IComponentAccessor {
public:
    ITransformComponent* GetTransform(entt::registry& registry, entt::entity entity) override {
        if (registry.all_of<TransformComponent>(entity)) {
            auto& transform = registry.get<TransformComponent>(entity);
            // Return adapter that bridges Engine component to Graphics interface
            return new TransformComponentAdapter(transform);
        }
        return nullptr;
    }
};

// Adapter bridges the gap
class TransformComponentAdapter : public ITransformComponent {
public:
    TransformComponentAdapter(TransformComponent& engineTransform) 
        : m_EngineTransform(engineTransform) {}
    
    glm::mat4 GetTransform() const override { 
        return m_EngineTransform.GetTransform(); 
    }
    
    glm::vec3 GetPosition() const override { 
        return m_EngineTransform.Translation; 
    }
    
private:
    TransformComponent& m_EngineTransform;
};
```

This pattern allows:
- **Graphics library** to be completely independent of Engine implementation
- **Engine** to be swapped out without touching Graphics code
- **Testing** each layer independently
- **Future compatibility** when adding new component types

---

## Adding New Features

### 1. Adding New Command Types

Commands are the atomic operations that the GPU performs. Adding new command types is the most common way to extend rendering functionality.

**Example: Adding Compute Shader Support**

**Step 1: Define the Command Structure**

```cpp
// In lib/Graphics/include/Core/RenderCommandBuffer.h
namespace RenderCommands {
    struct ComputeDispatchData {
        std::shared_ptr<ComputeShader> shader;
        uint32_t groupsX, groupsY, groupsZ;
        
        // Optional: Pre-compute binding data for performance
        uint32_t uniformBufferHandle = 0;
        uint32_t storageBufferHandle = 0;
    };
}

// Update the variant to include new command
using VariantRenderCommand = std::variant<
    RenderCommands::ClearData,
    RenderCommands::BindShaderData,
    RenderCommands::SetUniformsData,
    RenderCommands::BindTexturesData,
    RenderCommands::DrawElementsData,
    RenderCommands::BindSSBOData,
    RenderCommands::DrawElementsInstancedData,
    RenderCommands::ComputeDispatchData  // <-- New command type
>;
```

**Step 2: Implement Command Execution**

```cpp
// In lib/Graphics/src/Core/RenderCommandBuffer.cpp
void RenderCommandBuffer::ExecuteCommand(const RenderCommands::ComputeDispatchData& cmd) {
    if (!cmd.shader) {
        std::cerr << "Warning: Null compute shader in ComputeDispatchData" << std::endl;
        return;
    }
    
    // Bind compute shader
    cmd.shader->use();
    
    // Bind storage buffers if specified
    if (cmd.storageBufferHandle != 0) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cmd.storageBufferHandle);
    }
    
    // Dispatch compute work
    glDispatchCompute(cmd.groupsX, cmd.groupsY, cmd.groupsZ);
    
    // Ensure compute completion before subsequent operations
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
```

**Step 3: Create High-Level API**

```cpp
// Create a new coordinator for compute operations
// lib/Graphics/include/Rendering/ComputeRenderer.h
class ComputeRenderer {
public:
    void DispatchCompute(std::shared_ptr<ComputeShader> shader,
                         uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ,
                         GLuint storageBuffer = 0) {
        CommandSortKey sortKey;
        sortKey.pass = 255;  // Compute pass (before all rendering)
        sortKey.material = shader->GetID();
        sortKey.mesh = 0;
        sortKey.instance = 0;
        
        ComputeDispatchData cmd;
        cmd.shader = shader;
        cmd.groupsX = groupsX;
        cmd.groupsY = groupsY;
        cmd.groupsZ = groupsZ;
        cmd.storageBufferHandle = storageBuffer;
        
        Renderer::Get().Submit(cmd, sortKey);
    }
};
```

**Step 4: Integrate with Scene Rendering**

```cpp
// In lib/Graphics/include/Scene/SceneRenderer.h
class SceneRenderer {
private:
    std::unique_ptr<ComputeRenderer> m_ComputeRenderer;  // Add new coordinator
    
public:
    ComputeRenderer* GetComputeRenderer() const { return m_ComputeRenderer.get(); }
};

// In SceneRenderer constructor
SceneRenderer::SceneRenderer() {
    // ... existing coordinators ...
    m_ComputeRenderer = std::make_unique<ComputeRenderer>();
}

// In render pipeline (before geometry pass)
void SceneRenderer::Render() {
    m_Pipeline->Execute();
}

// In GeometryPass::Execute()
void GeometryPass::Execute() {
    // Compute operations run first (pass = 255)
    // Then regular rendering (pass = 0)
}
```

### 2. Adding New Rendering Coordinators

Coordinators are the bridge between data sources and the command system.

**Example: Adding a Particle System**

**Step 1: Define Particle Data Structure**

```cpp
// lib/Graphics/include/Rendering/ParticleRenderer.h
class ParticleRenderer {
public:
    struct ParticleData {
        glm::vec3 position;     // 12 bytes
        float life;             // 4 bytes
        glm::vec3 velocity;     // 12 bytes
        float size;             // 4 bytes
        glm::vec4 color;        // 16 bytes
        uint32_t textureIndex;  // 4 bytes
        uint32_t padding[3];    // 12 bytes padding for std430 alignment
        // Total: 64 bytes per particle (GPU-friendly)
    };
    
    struct ParticleEmitter {
        std::string name;
        glm::vec3 position;
        glm::vec3 direction;
        float emissionRate;
        float lifetime;
        uint32_t maxParticles;
        
        // Render data
        std::shared_ptr<Shader> shader;
        std::vector<Texture> textures;
        
        // Runtime data
        std::vector<ParticleData> particles;
        std::unique_ptr<ShaderStorageBuffer> particleSSBO;
        float timeSinceLastEmit = 0.0f;
    };
    
    void AddEmitter(const std::string& name, const ParticleEmitter& emitter);
    void UpdateParticles(float deltaTime);
    void Render(Scene* scene, Camera& camera);
    
private:
    std::unordered_map<std::string, ParticleEmitter> m_Emitters;
    
    void UpdateEmitter(ParticleEmitter& emitter, float deltaTime);
    void RenderEmitter(const ParticleEmitter& emitter, Camera& camera);
};
```

**Step 2: Implement Update Logic**

```cpp
// lib/Graphics/src/Rendering/ParticleRenderer.cpp
void ParticleRenderer::UpdateParticles(float deltaTime) {
    for (auto& [name, emitter] : m_Emitters) {
        UpdateEmitter(emitter, deltaTime);
    }
}

void ParticleRenderer::UpdateEmitter(ParticleEmitter& emitter, float deltaTime) {
    // Update existing particles
    for (auto it = emitter.particles.begin(); it != emitter.particles.end();) {
        ParticleData& particle = *it;
        
        // Update physics
        particle.position += particle.velocity * deltaTime;
        particle.life -= deltaTime;
        
        // Remove dead particles
        if (particle.life <= 0.0f) {
            it = emitter.particles.erase(it);
        } else {
            ++it;
        }
    }
    
    // Emit new particles
    emitter.timeSinceLastEmit += deltaTime;
    float emitInterval = 1.0f / emitter.emissionRate;
    
    while (emitter.timeSinceLastEmit >= emitInterval && 
           emitter.particles.size() < emitter.maxParticles) {
        
        ParticleData newParticle;
        newParticle.position = emitter.position;
        newParticle.velocity = emitter.direction + RandomVec3(-0.5f, 0.5f);
        newParticle.life = emitter.lifetime;
        newParticle.size = 1.0f;
        newParticle.color = glm::vec4(1.0f);
        newParticle.textureIndex = 0;
        
        emitter.particles.push_back(newParticle);
        emitter.timeSinceLastEmit -= emitInterval;
    }
    
    // Upload to GPU
    if (emitter.particleSSBO) {
        emitter.particleSSBO->SetData(emitter.particles.data(), 
                                      emitter.particles.size() * sizeof(ParticleData));
    }
}
```

**Step 3: Implement Rendering**

```cpp
void ParticleRenderer::Render(Scene* scene, Camera& camera) {
    for (const auto& [name, emitter] : m_Emitters) {
        if (!emitter.particles.empty()) {
            RenderEmitter(emitter, camera);
        }
    }
}

void ParticleRenderer::RenderEmitter(const ParticleEmitter& emitter, Camera& camera) {
    CommandSortKey sortKey;
    sortKey.pass = 2;  // Transparent pass (after opaque)
    sortKey.material = emitter.shader->GetID();
    sortKey.mesh = 0;  // Particles use point sprites or instanced quads
    sortKey.instance = 0;
    
    // Submit rendering commands
    Renderer::Get().Submit(BindShaderData{emitter.shader}, sortKey);
    
    // Bind particle data SSBO
    Renderer::Get().Submit(BindSSBOData{emitter.particleSSBO->GetHandle(), 0}, sortKey);
    
    // Set camera uniforms
    SetUniformsData uniforms;
    uniforms.shader = emitter.shader;
    uniforms.modelMatrix = glm::mat4(1.0f);  // Identity
    uniforms.viewMatrix = camera.GetViewMatrix();
    uniforms.projectionMatrix = camera.GetProjectionMatrix();
    uniforms.cameraPosition = camera.GetPosition();
    Renderer::Get().Submit(uniforms, sortKey);
    
    // Bind textures
    Renderer::Get().Submit(BindTexturesData{emitter.textures, emitter.shader}, sortKey);
    
    // Draw particles (instanced point sprites)
    Renderer::Get().Submit(DrawElementsInstancedData{
        /* VAO = */ GetParticleVAO(),  // Single point or quad
        /* indexCount = */ 6,           // 2 triangles for quad
        /* instanceCount = */ static_cast<uint32_t>(emitter.particles.size()),
        /* baseInstance = */ 0
    }, sortKey);
}
```

### 3. Adding New Render Passes

Render passes enable multi-pass rendering techniques like shadow mapping, deferred rendering, and post-processing.

**Example: Adding Shadow Mapping**

**Step 1: Create Shadow Pass Class**

```cpp
// lib/Graphics/include/Pipeline/ShadowPass.h
class ShadowPass : public RenderPass {
public:
    ShadowPass(uint32_t shadowMapSize = 2048, uint32_t cascadeCount = 1);
    ~ShadowPass();
    
    void Begin() override;
    void Execute() override;  
    void End() override;
    
    // Access for subsequent passes
    GLuint GetShadowMapTexture() const { return m_ShadowMapTexture; }
    glm::mat4 GetLightSpaceMatrix() const { return m_LightSpaceMatrix; }
    
    // Configuration
    void SetLightDirection(const glm::vec3& direction) { m_LightDirection = direction; }
    void SetShadowBounds(float nearPlane, float farPlane, float orthoSize);
    
private:
    // Shadow map configuration
    uint32_t m_ShadowMapSize;
    uint32_t m_CascadeCount;
    
    // OpenGL resources
    GLuint m_ShadowMapFBO;
    GLuint m_ShadowMapTexture;
    
    // Light configuration
    glm::vec3 m_LightDirection;
    glm::mat4 m_LightSpaceMatrix;
    
    // Shadow render state
    float m_NearPlane = 1.0f;
    float m_FarPlane = 100.0f;
    float m_OrthoSize = 50.0f;
    
    void CreateFramebuffer();
    void UpdateLightMatrix();
};
```

**Step 2: Implement Shadow Pass**

```cpp
// lib/Graphics/src/Pipeline/ShadowPass.cpp
ShadowPass::ShadowPass(uint32_t shadowMapSize, uint32_t cascadeCount)
    : RenderPass("ShadowPass", {}), m_ShadowMapSize(shadowMapSize), 
      m_CascadeCount(cascadeCount), m_LightDirection(0.0f, -1.0f, 0.0f) {
    CreateFramebuffer();
}

void ShadowPass::CreateFramebuffer() {
    // Create shadow map texture
    glGenTextures(1, &m_ShadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, m_ShadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, 
                 m_ShadowMapSize, m_ShadowMapSize, 0, 
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    
    // Configure shadow map sampling
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
    // Set border color to maximum depth (no shadow)
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    // Create framebuffer
    glGenFramebuffers(1, &m_ShadowMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
                          GL_TEXTURE_2D, m_ShadowMapTexture, 0);
    
    // No color attachment needed for depth-only rendering
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Shadow map framebuffer not complete!" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowPass::Begin() {
    UpdateLightMatrix();
    
    // Bind shadow map framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowMapFBO);
    glViewport(0, 0, m_ShadowMapSize, m_ShadowMapSize);
    
    // Clear depth buffer
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // Configure depth testing for shadow rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Optional: Enable front-face culling to reduce peter panning
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
}

void ShadowPass::Execute() {
    // The render function will be set by SceneRenderer
    if (m_RenderFunction) {
        m_RenderFunction();
    }
}

void ShadowPass::End() {
    // Restore normal culling
    glCullFace(GL_BACK);
    
    // Unbind framebuffer (return to default)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowPass::UpdateLightMatrix() {
    // Create orthographic light-space matrix
    glm::mat4 lightView = glm::lookAt(
        -m_LightDirection * m_FarPlane * 0.5f,  // Light position (far from scene)
        glm::vec3(0.0f, 0.0f, 0.0f),            // Look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)             // Up vector
    );
    
    glm::mat4 lightProjection = glm::ortho(
        -m_OrthoSize, m_OrthoSize,    // Left, right
        -m_OrthoSize, m_OrthoSize,    // Bottom, top
        m_NearPlane, m_FarPlane       // Near, far
    );
    
    m_LightSpaceMatrix = lightProjection * lightView;
}
```

**Step 3: Integrate with Scene Rendering**

```cpp
// In SceneRenderer::InitializePipeline()
void SceneRenderer::InitializePipeline() {
    m_Pipeline = std::make_unique<RenderPipeline>("MainPipeline");
    
    // Create shadow pass
    auto shadowPass = std::make_shared<ShadowPass>(2048);
    shadowPass->SetLightDirection(glm::vec3(0.5f, -1.0f, -0.3f));
    shadowPass->SetRenderFunction([this]() {
        // Render scene from light's perspective for shadow map
        // Use shadow-specific shader that only outputs depth
        m_MeshRenderer->RenderShadowPass(m_Scene.get(), /* light camera */);
        m_InstancedRenderer->RenderShadowPass(m_Scene.get(), /* light camera */);
    });
    
    // Create main geometry pass  
    auto geometryPass = std::make_shared<GeometryPass>(FBOSpecs{1280, 720});
    geometryPass->SetRenderFunction([this, shadowPass]() {
        // Bind shadow map for sampling in main shaders
        glActiveTexture(GL_TEXTURE31);  // Use high texture unit
        glBindTexture(GL_TEXTURE_2D, shadowPass->GetShadowMapTexture());
        
        // Set light-space matrix uniform for shadow coordinate calculation
        // (This would be done in the renderer coordinators)
        
        // Regular scene rendering with shadow sampling
        m_FrustumCuller->CullAgainstCamera(m_Scene.get(), *m_Camera);
        m_MeshRenderer->Render(m_Scene.get(), *m_Camera);
        m_InstancedRenderer->Render(m_Scene.get(), *m_Camera);
    });
    
    // Add passes to pipeline (order matters!)
    m_Pipeline->AddPass(shadowPass);    // Shadow pass first
    m_Pipeline->AddPass(geometryPass);  // Main rendering second
}
```

### 4. Adding New Resource Types

**Example: Adding Cubemap Support**

```cpp
// lib/Graphics/include/Resources/Cubemap.h
class Cubemap {
public:
    Cubemap(const std::array<std::string, 6>& faces);
    ~Cubemap();
    
    void bind() const { glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID); }
    GLuint GetID() const { return m_ID; }
    
private:
    GLuint m_ID;
    void LoadCubemapFace(const std::string& path, GLenum target);
};

// Usage in skybox rendering
class SkyboxRenderer {
public:
    void SetCubemap(std::shared_ptr<Cubemap> cubemap) { m_Cubemap = cubemap; }
    void Render(Camera& camera);
    
private:
    std::shared_ptr<Cubemap> m_Cubemap;
    std::shared_ptr<Shader> m_SkyboxShader;
    GLuint m_SkyboxVAO;
};
```

---

## Performance Optimization Guide

### Profiling and Measurement

**1. Frame Time Profiling**

```cpp
// lib/Graphics/include/Utility/Profiler.h
class GPUProfiler {
public:
    void BeginFrame();
    void EndFrame();
    
    void BeginRegion(const std::string& name);
    void EndRegion(const std::string& name);
    
    float GetFrameTime() const;
    float GetRegionTime(const std::string& name) const;
    
private:
    struct ProfileRegion {
        GLuint queryBegin, queryEnd;
        std::string name;
        float lastTime = 0.0f;
    };
    
    std::unordered_map<std::string, ProfileRegion> m_Regions;
    GLuint m_FrameQueryBegin, m_FrameQueryEnd;
};

// Usage in rendering coordinators
void MeshRenderer::Render(Scene* scene, Camera& camera) {
    Profiler::Get().BeginRegion("MeshRenderer");
    
    // ... rendering code ...
    
    Profiler::Get().EndRegion("MeshRenderer");
}
```

**2. Memory Usage Tracking**

```cpp
class MemoryTracker {
public:
    void TrackAllocation(const std::string& category, size_t bytes);
    void TrackDeallocation(const std::string& category, size_t bytes);
    
    struct MemoryStats {
        size_t currentBytes = 0;
        size_t peakBytes = 0;
        size_t totalAllocations = 0;
        size_t totalDeallocations = 0;
    };
    
    MemoryStats GetStats(const std::string& category) const;
    void PrintSummary() const;
    
private:
    std::unordered_map<std::string, MemoryStats> m_Stats;
};

// Automatic tracking with RAII
class TrackedBuffer {
public:
    TrackedBuffer(size_t size, const std::string& category) 
        : m_Size(size), m_Category(category) {
        glGenBuffers(1, &m_ID);
        glBindBuffer(GL_ARRAY_BUFFER, m_ID);
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
        
        MemoryTracker::Get().TrackAllocation(category, size);
    }
    
    ~TrackedBuffer() {
        glDeleteBuffers(1, &m_ID);
        MemoryTracker::Get().TrackDeallocation(m_Category, m_Size);
    }
    
private:
    GLuint m_ID;
    size_t m_Size;
    std::string m_Category;
};
```

### CPU Optimization Strategies

**1. Command Buffer Optimization**

```cpp
// Pre-allocate command buffer to avoid reallocations
class RenderCommandBuffer {
public:
    void Reserve(size_t expectedCommands) {
        m_Commands.reserve(expectedCommands);
    }
    
    void Clear() {
        // Don't shrink capacity, just reset size
        m_Commands.clear();  // Keeps allocated memory
    }
    
private:
    std::vector<SortableCommand> m_Commands;
};

// Usage: Reserve space at application startup
void Application::Initialize() {
    // For a scene with ~1000 objects * 4 commands each
    Renderer::Get().GetCommandBuffer().Reserve(4000);
}
```

**2. Memory Pool Allocation**

```cpp
// Pool allocator for frequently created/destroyed objects
template<typename T, size_t PoolSize = 1024>
class ObjectPool {
public:
    T* Acquire() {
        if (m_FreeList.empty()) {
            // Pool exhausted - allocate more or reuse oldest
            return new T();  // Fallback to heap allocation
        }
        
        T* obj = m_FreeList.back();
        m_FreeList.pop_back();
        return obj;
    }
    
    void Release(T* obj) {
        if (obj && m_FreeList.size() < PoolSize) {
            // Reset object state
            obj->~T();
            new(obj) T();  // Placement new for reinitialization
            m_FreeList.push_back(obj);
        } else {
            delete obj;  // Pool full, delete normally
        }
    }
    
private:
    std::vector<T*> m_FreeList;
    std::array<T, PoolSize> m_Pool;
};
```

### GPU Optimization Strategies

**1. SSBO Optimization for Instancing**

```cpp
// Optimize SSBO updates with orphaning technique
class OptimizedInstancedRenderer {
private:
    struct BufferSlot {
        GLuint ssbo;
        size_t size;
        bool inUse;
    };
    
    std::array<BufferSlot, 3> m_BufferSlots;  // Triple buffering
    size_t m_CurrentSlot = 0;
    
public:
    void UpdateInstanceData(const std::vector<InstanceData>& instances) {
        size_t dataSize = instances.size() * sizeof(InstanceData);
        BufferSlot& slot = m_BufferSlots[m_CurrentSlot];
        
        if (slot.size < dataSize) {
            // Resize buffer using orphaning technique
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, slot.ssbo);
            glBufferData(GL_SHADER_STORAGE_BUFFER, dataSize, nullptr, GL_STREAM_DRAW);  // Orphan
            glBufferData(GL_SHADER_STORAGE_BUFFER, dataSize, instances.data(), GL_STREAM_DRAW);
            slot.size = dataSize;
        } else {
            // Update existing buffer
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, slot.ssbo);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, dataSize, instances.data());
        }
        
        m_CurrentSlot = (m_CurrentSlot + 1) % 3;  // Round robin
    }
};
```

**2. Texture Streaming for Large Scenes**

```cpp
// Automatic LOD based on distance
class TextureLODManager {
public:
    struct TextureLOD {
        GLuint highRes, mediumRes, lowRes;
        float distance;
        uint32_t currentLOD = 0;
    };
    
    void UpdateLOD(const glm::vec3& cameraPos) {
        for (auto& [textureID, lod] : m_TextureLODs) {
            uint32_t newLOD = CalculateLOD(lod.distance, cameraPos);
            if (newLOD != lod.currentLOD) {
                SwitchLOD(textureID, newLOD);
                lod.currentLOD = newLOD;
            }
        }
    }
    
private:
    std::unordered_map<uint32_t, TextureLOD> m_TextureLODs;
    
    uint32_t CalculateLOD(float distance, const glm::vec3& cameraPos) {
        if (distance < 10.0f) return 0;  // High res
        if (distance < 50.0f) return 1;  // Medium res
        return 2;  // Low res
    }
};
```

---

## Debugging and Profiling

### Debug Build Configuration

```cmake
# CMakeLists.txt - Add debug options
option(GRAPHICS_DEBUG "Enable graphics debugging" OFF)
option(GRAPHICS_VERBOSE "Enable verbose logging" OFF)

if(GRAPHICS_DEBUG)
    target_compile_definitions(GraphicsLib PRIVATE GRAPHICS_DEBUG_ENABLED)
endif()

if(GRAPHICS_VERBOSE)
    target_compile_definitions(GraphicsLib PRIVATE GRAPHICS_VERBOSE_ENABLED)
endif()
```

### OpenGL Debug Output

```cpp
// lib/Graphics/src/Core/GraphicsContext.cpp
void GraphicsContext::EnableDebugOutput() {
#ifdef GRAPHICS_DEBUG_ENABLED
    if (glDebugMessageCallback) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        
        glDebugMessageCallback(OpenGLDebugCallback, nullptr);
        
        // Control message severity
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 
                             0, nullptr, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM,
                             0, nullptr, GL_TRUE);
        
        std::cout << "OpenGL debug output enabled" << std::endl;
    }
#endif
}

void GLAPIENTRY OpenGLDebugCallback(GLenum source, GLenum type, GLuint id,
                                   GLenum severity, GLsizei length,
                                   const GLchar* message, const void* userParam) {
    std::string sourceStr = GetDebugSourceString(source);
    std::string typeStr = GetDebugTypeString(type);
    std::string severityStr = GetDebugSeverityString(severity);
    
    std::cout << "[OpenGL " << severityStr << "] " << sourceStr << " - " 
              << typeStr << ": " << message << std::endl;
    
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        // Break on high severity errors in debug builds
        #ifdef _DEBUG
        __debugbreak();  // MSVC
        // __builtin_trap();  // GCC/Clang
        #endif
    }
}
```

### Command Buffer Debugging

```cpp
// lib/Graphics/include/Core/RenderCommandBuffer.h
class RenderCommandBuffer {
public:
    void EnableDebugging(bool enable) { m_DebugEnabled = enable; }
    
    void Submit(const VariantRenderCommand& command, const CommandSortKey& sortKey) {
        #ifdef GRAPHICS_DEBUG_ENABLED
        if (m_DebugEnabled) {
            LogCommandSubmission(command, sortKey);
        }
        #endif
        
        m_Commands.emplace_back(SortableCommand{command, sortKey});
    }
    
    void PrintCommandSummary() const;
    void ValidateCommandSequence() const;
    
private:
    bool m_DebugEnabled = false;
    
    void LogCommandSubmission(const VariantRenderCommand& command, 
                             const CommandSortKey& sortKey);
};

void RenderCommandBuffer::PrintCommandSummary() const {
    std::cout << "=== Command Buffer Summary ===" << std::endl;
    std::cout << "Total commands: " << m_Commands.size() << std::endl;
    
    // Count command types
    std::map<std::string, int> commandCounts;
    for (const auto& cmd : m_Commands) {
        std::visit([&](const auto& c) {
            commandCounts[typeid(c).name()]++;
        }, cmd.command);
    }
    
    for (const auto& [type, count] : commandCounts) {
        std::cout << "  " << type << ": " << count << std::endl;
    }
    
    // Analyze sort key distribution
    std::map<uint8_t, int> passCounts;
    std::map<uint32_t, int> materialCounts;
    
    for (const auto& cmd : m_Commands) {
        passCounts[cmd.key.pass]++;
        materialCounts[cmd.key.material]++;
    }
    
    std::cout << "Pass distribution:" << std::endl;
    for (const auto& [pass, count] : passCounts) {
        std::cout << "  Pass " << (int)pass << ": " << count << " commands" << std::endl;
    }
    
    std::cout << "Unique materials: " << materialCounts.size() << std::endl;
}
```

### Visual Debugging Tools

```cpp
// lib/Graphics/include/Utility/DebugRenderer.h
class DebugRenderer {
public:
    void DrawLine(const glm::vec3& start, const glm::vec3& end, 
                  const glm::vec3& color = glm::vec3(1.0f));
    void DrawWireCube(const glm::vec3& center, const glm::vec3& size,
                      const glm::vec3& color = glm::vec3(1.0f));
    void DrawWireSphere(const glm::vec3& center, float radius,
                        const glm::vec3& color = glm::vec3(1.0f));
    void DrawFrustum(const glm::mat4& viewProjection,
                     const glm::vec3& color = glm::vec3(1.0f, 1.0f, 0.0f));
    
    void DrawText(const std::string& text, const glm::vec2& position,
                  const glm::vec3& color = glm::vec3(1.0f));
    
    void Render(Camera& camera);
    void Clear();
    
private:
    struct DebugLine {
        glm::vec3 start, end, color;
    };
    
    std::vector<DebugLine> m_Lines;
    std::shared_ptr<Shader> m_LineShader;
    GLuint m_LineVAO, m_LineVBO;
};

// Usage in rendering coordinators
void FrustumCuller::CullAgainstCamera(Scene* scene, Camera& camera) {
    #ifdef GRAPHICS_DEBUG_ENABLED
    if (m_DebugVisualization) {
        DebugRenderer::Get().DrawFrustum(
            camera.GetProjectionMatrix() * camera.GetViewMatrix(),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }
    #endif
    
    // ... culling logic ...
}
```

---

## Best Practices

### Code Organization

**1. Use Namespace Organization**

```cpp
// Organize code in logical namespaces
namespace Graphics {
    namespace Core {
        class Renderer { /* ... */ };
        class RenderCommandBuffer { /* ... */ };
    }
    
    namespace Resources {
        class Shader { /* ... */ };
        class Texture { /* ... */ };
    }
    
    namespace Rendering {
        class MeshRenderer { /* ... */ };
        class InstancedRenderer { /* ... */ };
    }
    
    namespace Utility {
        class Camera { /* ... */ };
        class Profiler { /* ... */ };
    }
}
```

**2. RAII for Resource Management**

```cpp
// Always use RAII for GPU resources
class Buffer {
public:
    Buffer(size_t size, GLenum usage) : m_Size(size) {
        glGenBuffers(1, &m_ID);
        glBindBuffer(GL_ARRAY_BUFFER, m_ID);
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, usage);
    }
    
    ~Buffer() {
        if (m_ID != 0) {
            glDeleteBuffers(1, &m_ID);
        }
    }
    
    // Move semantics
    Buffer(Buffer&& other) noexcept : m_ID(other.m_ID), m_Size(other.m_Size) {
        other.m_ID = 0;
        other.m_Size = 0;
    }
    
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            if (m_ID != 0) {
                glDeleteBuffers(1, &m_ID);
            }
            m_ID = other.m_ID;
            m_Size = other.m_Size;
            other.m_ID = 0;
            other.m_Size = 0;
        }
        return *this;
    }
    
    // Delete copy operations (move-only type)
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    
private:
    GLuint m_ID = 0;
    size_t m_Size = 0;
};
```

**3. Error Handling Patterns**

```cpp
// Use Result<T> or Optional<T> for error handling
template<typename T>
class Result {
public:
    static Result Success(T value) {
        return Result(std::move(value), "");
    }
    
    static Result Error(const std::string& message) {
        return Result({}, message);
    }
    
    bool IsSuccess() const { return m_Error.empty(); }
    bool IsError() const { return !m_Error.empty(); }
    
    const T& GetValue() const { 
        assert(IsSuccess()); 
        return m_Value; 
    }
    
    const std::string& GetError() const { 
        assert(IsError()); 
        return m_Error; 
    }
    
private:
    Result(T value, const std::string& error) 
        : m_Value(std::move(value)), m_Error(error) {}
    
    T m_Value;
    std::string m_Error;
};

// Usage
Result<std::shared_ptr<Shader>> LoadShader(const std::string& path) {
    // ... loading logic ...
    if (compilationSucceeded) {
        return Result<std::shared_ptr<Shader>>::Success(shader);
    } else {
        return Result<std::shared_ptr<Shader>>::Error("Compilation failed: " + errorLog);
    }
}

// Client code
auto result = LoadShader("shader.glsl");
if (result.IsSuccess()) {
    auto shader = result.GetValue();
    // ... use shader ...
} else {
    std::cerr << "Shader loading failed: " << result.GetError() << std::endl;
}
```

### Performance Best Practices

**1. Minimize State Changes**

```cpp
// Bad: Frequent state changes
for (int i = 0; i < objects.size(); ++i) {
    glUseProgram(objects[i].shader);        // State change
    glBindTexture(GL_TEXTURE_2D, objects[i].texture);  // State change
    glDrawElements(GL_TRIANGLES, objects[i].indexCount, GL_UNSIGNED_INT, nullptr);
}

// Good: Batch by state
std::sort(objects.begin(), objects.end(), [](const Object& a, const Object& b) {
    if (a.shader != b.shader) return a.shader < b.shader;
    return a.texture < b.texture;
});

GLuint currentShader = 0, currentTexture = 0;
for (const auto& obj : objects) {
    if (obj.shader != currentShader) {
        glUseProgram(obj.shader);
        currentShader = obj.shader;
    }
    if (obj.texture != currentTexture) {
        glBindTexture(GL_TEXTURE_2D, obj.texture);
        currentTexture = obj.texture;
    }
    glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, nullptr);
}
```

**2. Prefer Data-Oriented Design**

```cpp
// Bad: Object-oriented (poor cache usage)
class GameObject {
    Transform transform;
    Renderer renderer;
    Physics physics;
    AI ai;
    
    void Update() {
        // Touches all components, poor cache usage
    }
};

std::vector<GameObject> objects;

// Good: Component arrays (excellent cache usage)
struct TransformSystem {
    std::vector<glm::mat4> transforms;
    
    void UpdateTransforms() {
        // Process all transforms sequentially - excellent cache usage
        for (auto& transform : transforms) {
            // ... update logic ...
        }
    }
};

struct RenderSystem {
    std::vector<MeshComponent> meshes;
    std::vector<MaterialComponent> materials;
    
    void Render() {
        // Process all mesh/material pairs - excellent cache usage
    }
};
```

---

## Common Patterns

### Resource Loading Pattern

```cpp
// Async resource loading with caching
template<typename T>
class ResourceCache {
public:
    std::future<std::shared_ptr<T>> LoadAsync(const std::string& path) {
        // Check cache first
        {
            std::lock_guard<std::mutex> lock(m_CacheMutex);
            auto it = m_Cache.find(path);
            if (it != m_Cache.end()) {
                // Return cached resource
                std::promise<std::shared_ptr<T>> promise;
                promise.set_value(it->second);
                return promise.get_future();
            }
        }
        
        // Load asynchronously
        return std::async(std::launch::async, [this, path]() {
            auto resource = this->LoadImpl(path);
            
            // Cache the loaded resource
            {
                std::lock_guard<std::mutex> lock(m_CacheMutex);
                m_Cache[path] = resource;
            }
            
            return resource;
        });
    }
    
protected:
    virtual std::shared_ptr<T> LoadImpl(const std::string& path) = 0;
    
private:
    std::unordered_map<std::string, std::shared_ptr<T>> m_Cache;
    std::mutex m_CacheMutex;
};
```

### Component Registration Pattern

```cpp
// Automatic component registration
class ComponentRegistry {
public:
    template<typename T>
    void RegisterComponent() {
        static_assert(std::is_base_of_v<IComponent, T>, 
                      "T must derive from IComponent");
        
        ComponentInfo info;
        info.name = typeid(T).name();
        info.size = sizeof(T);
        info.createFunc = []() -> std::unique_ptr<IComponent> {
            return std::make_unique<T>();
        };
        
        m_Components[info.name] = info;
    }
    
    std::unique_ptr<IComponent> Create(const std::string& name) {
        auto it = m_Components.find(name);
        if (it != m_Components.end()) {
            return it->second.createFunc();
        }
        return nullptr;
    }
    
private:
    struct ComponentInfo {
        std::string name;
        size_t size;
        std::function<std::unique_ptr<IComponent>()> createFunc;
    };
    
    std::unordered_map<std::string, ComponentInfo> m_Components;
};

// Auto-registration using static initialization
template<typename T>
struct ComponentRegistrar {
    ComponentRegistrar() {
        ComponentRegistry::Get().RegisterComponent<T>();
    }
};

#define REGISTER_COMPONENT(T) \
    static ComponentRegistrar<T> g_##T##_registrar;

// Usage
class MeshComponent : public IComponent { /* ... */ };
REGISTER_COMPONENT(MeshComponent);
```

---

## Troubleshooting

### Common Issues and Solutions

**1. Bindless Texture Fallback Issues**

**Problem**: Bindless shaders fail when bindless textures aren't supported.

**Solution**: Implement shader selection based on capabilities:

```cpp
// In Application::LoadResources()
void LoadResources() override {
    bool bindlessSupported = BindlessTextureHelper::IsBindlessAvailable();
    
    if (bindlessSupported) {
        LoadShader("material", "assets/shaders/bindless.vert", "assets/shaders/bindless.frag");
        std::cout << "Using bindless texture pipeline" << std::endl;
    } else {
        LoadShader("material", "assets/shaders/basic.vert", "assets/shaders/basic.frag");
        std::cout << "Using traditional texture pipeline" << std::endl;
    }
}
```

**2. Command Sorting Performance**

**Problem**: Frame rate drops with large command counts.

**Solutions**:

```cpp
// Solution 1: Reserve command buffer capacity
void Application::Initialize() {
    Renderer::Get().GetCommandBuffer().Reserve(10000);  // Avoid reallocations
}

// Solution 2: Use parallel sort for large command counts
void RenderCommandBuffer::Sort() {
    if (m_Commands.size() > 1000) {
        // Use parallel sort for large arrays
        std::sort(std::execution::par_unseq, m_Commands.begin(), m_Commands.end());
    } else {
        std::sort(m_Commands.begin(), m_Commands.end());
    }
}
```

**3. SSBO Memory Issues**

**Problem**: GPU memory exhaustion with large instance counts.

**Solutions**:

```cpp
// Solution 1: Implement streaming
class StreamingInstancedRenderer {
private:
    static constexpr size_t MAX_INSTANCES_PER_BATCH = 10000;
    
public:
    void Render(const std::vector<InstanceData>& instances, Camera& camera) {
        for (size_t offset = 0; offset < instances.size(); offset += MAX_INSTANCES_PER_BATCH) {
            size_t count = std::min(MAX_INSTANCES_PER_BATCH, instances.size() - offset);
            RenderBatch(instances.data() + offset, count, camera);
        }
    }
};

// Solution 2: Use LOD to reduce instance count
void FilterInstancesByLOD(std::vector<InstanceData>& instances, const glm::vec3& cameraPos) {
    instances.erase(
        std::remove_if(instances.begin(), instances.end(), [&](const InstanceData& inst) {
            float distance = glm::length(GetPosition(inst.modelMatrix) - cameraPos);
            return distance > MAX_RENDER_DISTANCE;  // Cull distant instances
        }),
        instances.end()
    );
}
```

**4. Graphics Context Issues**

**Problem**: OpenGL calls fail silently or crash.

**Debug Steps**:

```cpp
// Enable debug output (see debugging section)
GraphicsContext::EnableDebugOutput();

// Check OpenGL errors after every call in debug builds
#ifdef GRAPHICS_DEBUG_ENABLED
#define GL_CALL(call) do { \
    call; \
    GLenum error = glGetError(); \
    if (error != GL_NO_ERROR) { \
        std::cerr << "OpenGL Error in " << __FILE__ << ":" << __LINE__ \
                  << " - " << #call << " - Error: " << error << std::endl; \
        assert(false); \
    } \
} while(0)
#else
#define GL_CALL(call) call
#endif

// Usage
GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
GL_CALL(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
```

This developer guide provides a comprehensive foundation for extending and improving the graphics framework. The modular architecture and clear extension points make it straightforward to add new features while maintaining performance and code quality.
