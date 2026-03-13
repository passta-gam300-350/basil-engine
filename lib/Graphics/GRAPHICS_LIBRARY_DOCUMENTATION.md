# Graphics Library Documentation
**Project:** C:\Users\thamk\gam300\lib\Graphics
**OpenGL Version:** Currently 4.3+ (targeting 4.5+ with DSA conversion)
**Last Updated:** March 12, 2026

---

## Table of Contents
1. [Architecture Overview](#architecture-overview)
2. [Directory Structure](#directory-structure)
3. [Core Components](#core-components)
4. [Class Reference](#class-reference)
5. [Rendering Pipeline](#rendering-pipeline)
6. [Resource Management](#resource-management)
7. [OpenGL Patterns](#opengl-patterns)
8. [Dependencies](#dependencies)
9. [Usage Examples](#usage-examples)

---

## Architecture Overview

### Design Philosophy
The Graphics library provides a modern C++ abstraction layer over OpenGL, following these principles:
- **RAII-based resource management** - OpenGL objects wrapped in C++ classes with automatic cleanup
- **Multi-threaded rendering** - Dual OpenGL contexts (editor thread + engine thread) with shared resources
- **Command buffer pattern** - Deferred execution for render commands
- **Material system** - 3-tier material architecture (Material → MaterialInstance → MaterialPropertyBlock)
- **Data-driven rendering** - Scene graph with render passes

### Key Features
- ✅ Deferred rendering pipeline
- ✅ Shadow mapping (directional + point lights with cubemaps)
- ✅ MSAA support
- ✅ Post-processing effects (bloom, tone mapping, FXAA)
- ✅ Instanced rendering via SSBOs
- ✅ Debug rendering (lines, wireframes, bounding boxes)
- ✅ Text rendering with font atlases
- ✅ Skybox rendering
- ✅ Model loading (via Assimp)

---

## Directory Structure

```
Graphics/
├── include/
│   ├── Buffer/              # OpenGL buffer and vertex array wrappers
│   │   ├── FrameBuffer.h
│   │   ├── CubemapFrameBuffer.h
│   │   ├── IndexBuffer.h
│   │   ├── VertexBuffer.h
│   │   ├── VertexArray.h
│   │   ├── UniformBuffer.h
│   │   └── ShaderStorageBuffer.h
│   │
│   ├── Resources/           # Asset management and material system
│   │   ├── Texture.h
│   │   ├── Shader.h
│   │   ├── Mesh.h
│   │   ├── Model.h
│   │   ├── Material.h
│   │   ├── MaterialInstance.h
│   │   ├── MaterialPropertyBlock.h
│   │   ├── TextureSlotManager.h
│   │   ├── FontAtlas.h
│   │   ├── ResourceManager.h
│   │   └── PrimitiveGenerator.h
│   │
│   ├── RenderPasses/        # Rendering stages
│   │   ├── MainRenderingPass.h
│   │   ├── ShadowMappingPass.h
│   │   ├── PointLightShadowMappingPass.h
│   │   ├── SkyboxRenderPass.h
│   │   ├── PostProcessingPass.h
│   │   ├── DebugRenderPass.h
│   │   ├── BloomPass.h
│   │   ├── FXAAPass.h
│   │   └── ... (15+ render passes)
│   │
│   ├── Renderer/            # High-level rendering systems
│   │   ├── SceneRenderer.h
│   │   ├── HUDRenderer.h
│   │   ├── TextRenderer.h
│   │   └── ForwardRenderer.h
│   │
│   ├── Scene/               # Scene graph components
│   │   ├── Scene.h
│   │   ├── Camera.h
│   │   ├── Light.h
│   │   └── SceneNode.h
│   │
│   └── Core/                # Core rendering utilities
│       ├── RenderCommandBuffer.h
│       ├── GraphicsContext.h
│       └── RenderState.h
│
└── src/                     # Implementation files (mirrors include structure)
    ├── Buffer/
    ├── Resources/
    ├── RenderPasses/
    ├── Renderer/
    ├── Scene/
    └── Core/
```

### File Count
- **Header files:** ~60 files
- **Source files:** ~53 files
- **Total lines of code:** ~15,000+ lines

---

## Core Components

### 1. Buffer Management (`include/Buffer/`)

#### **VertexBuffer**
**Purpose:** Wraps OpenGL VBO for vertex data storage
**File:** `include/Buffer/VertexBuffer.h`, `src/Buffer/VertexBuffer.cpp`
**Key Members:**
- `m_VBOHandle` - OpenGL buffer object ID

**Key Methods:**
```cpp
VertexBuffer(const void* data, uint32_t size);  // Create and upload data
void SetData(const void* data, uint32_t size);  // Update buffer data
void Bind() const;                              // Bind for rendering
void Unbind() const;                            // Unbind
```

**Current OpenGL Pattern:**
```cpp
glGenBuffers(1, &m_VBOHandle);
glBindBuffer(GL_ARRAY_BUFFER, m_VBOHandle);
glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
```

---

#### **IndexBuffer**
**Purpose:** Wraps OpenGL IBO/EBO for indexed rendering
**File:** `include/Buffer/IndexBuffer.h`, `src/Buffer/IndexBuffer.cpp`
**Key Members:**
- `m_IBOHandle` - OpenGL buffer object ID
- `m_Count` - Number of indices

**Key Methods:**
```cpp
IndexBuffer(const uint32_t* indices, uint32_t count);
uint32_t GetCount() const;
void Bind() const;
void Unbind() const;
```

---

#### **UniformBuffer**
**Purpose:** Wraps OpenGL UBO for shared shader uniforms
**File:** `include/Buffer/UniformBuffer.h`, `src/Buffer/UniformBuffer.cpp`
**Key Members:**
- `m_UBOHandle` - OpenGL buffer object ID
- `m_Binding` - Binding point index

**Key Methods:**
```cpp
UniformBuffer(uint32_t size, uint32_t binding);
void SetData(const void* data, uint32_t size, uint32_t offset = 0);
template<typename T> void Set(const T& data, uint32_t offset = 0);
```

**Usage Example:**
```cpp
// Create UBO bound to binding point 0
UniformBuffer cameraUBO(sizeof(CameraData), 0);

// Update data
CameraData data = {...};
cameraUBO.SetData(&data, sizeof(CameraData));
```

---

#### **ShaderStorageBuffer**
**Purpose:** Wraps OpenGL SSBO for large data arrays (e.g., instancing)
**File:** `include/Buffer/ShaderStorageBuffer.h`, `src/Buffer/ShaderStorageBuffer.cpp`
**Key Members:**
- `m_SSBOHandle` - OpenGL buffer object ID
- `m_Size` - Current buffer size
- `m_PersistentPtr` - Persistent mapped pointer (for streaming)

**Key Methods:**
```cpp
ShaderStorageBuffer(uint32_t size, const void* data = nullptr, GLenum usage = GL_DYNAMIC_DRAW);
void SetData(const void* data, uint32_t size, uint32_t offset = 0);
void Resize(uint32_t newSize);
void* Map(GLenum access);
void Unmap();
void CreatePersistentBuffer(uint32_t size);  // GL 4.4+ persistent mapping
```

**Usage (Instancing):**
```cpp
// 96 bytes per instance (mat4 transform + vec4 color)
ShaderStorageBuffer instanceSSBO(96 * 1000);
instanceSSBO.BindBase(0);  // Bind to SSBO binding point 0
```

---

#### **VertexArray**
**Purpose:** Wraps OpenGL VAO, manages vertex attribute layout
**File:** `include/Buffer/VertexArray.h`, `src/Buffer/VertexArray.cpp`
**Key Members:**
- `m_VAOHandle` - OpenGL VAO ID
- `m_VertexBuffers` - List of attached VBOs
- `m_IndexBuffer` - Attached IBO
- `m_VertexBufferIndex` - Track binding indices

**Key Methods:**
```cpp
VertexArray();
void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer, const BufferLayout& layout);
void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer);
void Bind() const;
```

**BufferLayout System:**
```cpp
BufferLayout layout = {
    { ShaderDataType::Float3, "a_Position" },
    { ShaderDataType::Float3, "a_Normal" },
    { ShaderDataType::Float2, "a_TexCoord" }
};
```

**Current Implementation (Lines 54-105):**
```cpp
void VertexArray::AddVertexBuffer(...) {
    Bind();  // glBindVertexArray
    vertexBuffer->Bind();  // glBindBuffer

    // Configure attributes
    for (auto& element : layout) {
        glEnableVertexAttribArray(index);
        glVertexAttribPointer(index, count, type, normalized, stride, offset);
        // OR glVertexAttribIPointer for integer types
    }
}
```

---

#### **FrameBuffer**
**Purpose:** Wraps OpenGL FBO with multiple color attachments and depth
**File:** `include/Buffer/FrameBuffer.h`, `src/Buffer/FrameBuffer.cpp`
**Key Members:**
- `m_FBOHandle` - OpenGL framebuffer ID
- `m_ColorAttachments` - Vector of color texture IDs
- `m_DepthAttachment` - Depth/stencil texture ID
- `m_Specification` - FBO configuration (size, attachments, MSAA)

**Key Methods:**
```cpp
FrameBuffer(const FramebufferSpecification& spec);
void Invalidate();  // Create/recreate FBO and attachments
void Bind();
void Unbind();
void Resize(uint32_t width, uint32_t height);
void ClearAttachment(int attachmentIndex, int value);
uint32_t GetColorAttachmentRendererID(int index = 0) const;
```

**FramebufferSpecification:**
```cpp
struct FramebufferSpecification {
    uint32_t Width, Height;
    uint32_t Samples = 1;  // MSAA sample count
    std::vector<FramebufferTextureFormat> Attachments;
    bool SwapChainTarget = false;
};
```

**Usage:**
```cpp
FramebufferSpecification spec;
spec.Width = 1920;
spec.Height = 1080;
spec.Attachments = {
    FramebufferTextureFormat::RGBA8,
    FramebufferTextureFormat::RGBA16F,
    FramebufferTextureFormat::Depth24Stencil8
};
FrameBuffer fbo(spec);
```

---

#### **CubemapFrameBuffer**
**Purpose:** Specialized FBO for cubemap rendering (shadow mapping)
**File:** `include/Buffer/CubemapFrameBuffer.h`, `src/Buffer/CubemapFrameBuffer.cpp`
**Key Members:**
- `m_FBOHandle` - OpenGL framebuffer ID
- `m_CubemapTexture` - Cubemap depth texture ID
- `m_Resolution` - Cubemap face resolution

**Key Methods:**
```cpp
CubemapFrameBuffer(unsigned int resolution);
void CreateCubemap();
void CreateFramebuffer();
void BindForRendering(unsigned int face);  // Bind specific cubemap face
```

**Usage (Point Light Shadows):**
```cpp
CubemapFrameBuffer shadowCubemap(1024);  // 1024x1024 per face
for (int face = 0; face < 6; ++face) {
    shadowCubemap.BindForRendering(face);
    // Render scene from light's perspective
}
```

---

### 2. Resource Management (`include/Resources/`)

#### **Shader**
**Purpose:** Shader program compilation, uniform management
**File:** `include/Resources/Shader.h`, `src/Resources/Shader.cpp`
**Key Members:**
- `m_ProgramID` - OpenGL shader program ID
- `m_UniformLocationCache` - Cache for uniform locations

**Key Methods:**
```cpp
Shader(const std::string& vertexPath, const std::string& fragmentPath);
void Bind() const;
void SetUniform*(const std::string& name, ...);  // SetInt, SetFloat, SetMat4, etc.
```

**Note:** Shader programs don't use DSA (already stateless for uniforms)

---

#### **Texture (TextureLoader)**
**Purpose:** Static utility for loading textures from disk
**File:** `include/Resources/Texture.h`, `src/Resources/Texture.cpp`
**Key Methods:**
```cpp
static unsigned int CreateGPUTexture(const TextureData& data, bool gamma);
static unsigned int CreateGPUCubemap(const std::vector<std::string>& faces);
static unsigned int CreateGPUTextureCompressed(const std::string& path);  // DDS
static TextureData LoadTextureFromFile(const std::string& path);
```

**Current Implementation (Lines 84-124):**
```cpp
unsigned int CreateGPUTexture(...) {
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, ...);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // ... more parameters
}
```

**Supported Formats:**
- PNG, JPG, TGA (via stb_image)
- DDS (compressed textures with mipmaps)
- HDR (for environment maps)

---

#### **TextureSlotManager**
**Purpose:** Manages texture unit bindings (0-31)
**File:** `include/Resources/TextureSlotManager.h`, `src/Resources/TextureSlotManager.cpp`
**Key Members:**
- `m_BoundTextures[32]` - Array of currently bound texture IDs
- `m_SlotUsed[32]` - Array of slot usage flags

**Key Methods:**
```cpp
void BindTextureToSlot(unsigned int textureID, int slot);
void UnbindTexture(int slot);
void UnbindAll();
int GetFreeSlot() const;
```

**Current Implementation (Lines 72-94):**
```cpp
void BindTextureToSlot(unsigned int textureID, int slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, textureID);
    m_BoundTextures[slot] = textureID;
    m_SlotUsed[slot] = true;
}
```

---

#### **Material System**

##### **Material**
**Purpose:** Defines shader + default properties for a material type
**File:** `include/Resources/Material.h`, `src/Resources/Material.cpp`
**Key Members:**
- `m_Shader` - Associated shader program
- `m_DefaultProperties` - Default texture/uniform values

**Key Methods:**
```cpp
Material(std::shared_ptr<Shader> shader);
void SetTexture(const std::string& name, unsigned int textureID);
void SetFloat(const std::string& name, float value);
```

##### **MaterialInstance**
**Purpose:** Instance of a material with overridden properties
**File:** `include/Resources/MaterialInstance.h`
**Key Members:**
- `m_BaseMaterial` - Parent material
- `m_OverrideProperties` - Per-instance overrides

##### **MaterialPropertyBlock**
**Purpose:** Per-draw call property overrides
**File:** `include/Resources/MaterialPropertyBlock.h`
**Usage:**
```cpp
Material mat(shader);
mat.SetTexture("u_Albedo", diffuseTexID);

MaterialInstance instance(mat);
instance.SetFloat("u_Metallic", 0.8f);  // Override

MaterialPropertyBlock block;
block.SetVec4("u_Color", glm::vec4(1, 0, 0, 1));  // Per-draw override
```

---

#### **Mesh**
**Purpose:** Geometry data + VAO wrapper
**File:** `include/Resources/Mesh.h`, `src/Resources/Mesh.cpp`
**Key Members:**
- `m_VertexArray` - VAO with vertex attributes
- `m_VertexBuffer` - VBO with vertex data
- `m_IndexBuffer` - IBO with index data
- `m_Vertices`, `m_Indices` - CPU-side data

**Vertex Structure:**
```cpp
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};
```

**Key Methods:**
```cpp
Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices, std::vector<Texture> textures);
void Draw(Shader& shader);
```

---

#### **Model**
**Purpose:** Container for multiple meshes (loaded via Assimp)
**File:** `include/Resources/Model.h`, `src/Resources/Model.cpp`
**Key Members:**
- `m_Meshes` - Vector of meshes
- `m_Directory` - Model file directory (for relative texture paths)

**Key Methods:**
```cpp
Model(const std::string& path);
void Draw(Shader& shader);
```

**Supported Formats:** FBX, OBJ, GLTF, DAE (via Assimp)

---

#### **FontAtlas**
**Purpose:** Bitmap font atlas for text rendering
**File:** `include/Resources/FontAtlas.h`, `src/Resources/FontAtlas.cpp`
**Key Members:**
- `m_AtlasTexture` - Packed glyph texture
- `m_GlyphData` - Per-character UV/metrics

**Usage:** Used by TextRenderer for UI text

---

#### **PrimitiveGenerator**
**Purpose:** Generates common geometric primitives
**File:** `include/Resources/PrimitiveGenerator.h`
**Key Methods:**
```cpp
static Mesh CreateCube();
static Mesh CreateSphere(int segments);
static Mesh CreatePlane();
static Mesh CreateCylinder();
```

---

### 3. Render Passes (`include/RenderPasses/`)

#### **Render Pass Architecture**
Each render pass encapsulates a rendering stage with its own FBO, shaders, and state.

**Base Pattern:**
```cpp
class RenderPass {
public:
    virtual void Initialize() = 0;
    virtual void Execute(Scene& scene) = 0;
    virtual void Resize(uint32_t width, uint32_t height) = 0;
};
```

#### **Key Render Passes:**

| Pass | Purpose | Output |
|------|---------|--------|
| `ShadowMappingPass` | Directional light shadow maps | Depth texture |
| `PointLightShadowMappingPass` | Point light shadow cubemaps | Depth cubemap |
| `MainRenderingPass` | Deferred geometry pass | G-buffer (albedo, normal, position) |
| `LightingPass` | Deferred lighting calculation | HDR color buffer |
| `SkyboxRenderPass` | Skybox rendering | Color buffer |
| `PostProcessingPass` | Tone mapping, gamma correction | LDR color buffer |
| `BloomPass` | Bloom effect (bright pass + blur) | Bloom texture |
| `FXAAPass` | Anti-aliasing | Final color buffer |
| `DebugRenderPass` | Debug lines, wireframes, AABBs | Overlay |

**Example (ShadowMappingPass.cpp):**
```cpp
void ShadowMappingPass::Initialize() {
    // Create depth-only FBO
    m_ShadowFBO = std::make_unique<FrameBuffer>(spec);
    m_ShadowShader = std::make_shared<Shader>("shadow.vert", "shadow.frag");
}

void ShadowMappingPass::Execute(Scene& scene) {
    m_ShadowFBO->Bind();
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render scene from light's perspective
    for (auto& mesh : scene.GetMeshes()) {
        mesh->Draw(*m_ShadowShader);
    }

    m_ShadowFBO->Unbind();
}
```

---

### 4. Renderer Systems (`include/Renderer/`)

#### **SceneRenderer**
**Purpose:** Main renderer, orchestrates all render passes
**File:** `include/Renderer/SceneRenderer.h`, `src/Renderer/SceneRenderer.cpp`
**Key Methods:**
```cpp
void Initialize();
void RenderScene(Scene& scene, Camera& camera);
void Resize(uint32_t width, uint32_t height);
```

**Rendering Order:**
1. Shadow passes (directional + point lights)
2. Main geometry pass (deferred)
3. Lighting pass
4. Skybox
5. Post-processing (bloom, tone mapping)
6. FXAA
7. Debug overlays

---

#### **HUDRenderer**
**Purpose:** 2D overlay rendering (UI elements)
**File:** `include/Renderer/HUDRenderer.h`

---

#### **TextRenderer**
**Purpose:** Text rendering using font atlases
**File:** `include/Renderer/TextRenderer.h`
**Key Methods:**
```cpp
void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color);
```

---

### 5. Scene Management (`include/Scene/`)

#### **Scene**
**Purpose:** Scene graph, manages entities and lights
**File:** `include/Scene/Scene.h`
**Key Members:**
- `m_Entities` - Renderable objects
- `m_Lights` - Light sources
- `m_Camera` - Active camera

---

#### **Camera**
**Purpose:** View and projection matrices
**File:** `include/Scene/Camera.h`
**Key Methods:**
```cpp
glm::mat4 GetViewMatrix() const;
glm::mat4 GetProjectionMatrix() const;
void SetPerspective(float fov, float aspect, float near, float far);
```

---

#### **Light**
**Purpose:** Light source data
**File:** `include/Scene/Light.h`
**Types:**
- Directional
- Point
- Spot

---

### 6. Core Utilities (`include/Core/`)

#### **RenderCommandBuffer**
**Purpose:** Deferred command execution
**File:** `include/Core/RenderCommandBuffer.h`, `src/Core/RenderCommandBuffer.cpp`
**Usage:**
```cpp
RenderCommandBuffer cmdBuffer;
cmdBuffer.AddCommand([&]() {
    mesh->Draw(shader);
});
cmdBuffer.Execute();
```

---

## Rendering Pipeline

### Frame Flow
```
1. Update Phase
   ├─> Update scene (transforms, animations)
   └─> Update camera

2. Shadow Pass
   ├─> Render directional light shadows
   └─> Render point light shadow cubemaps

3. Geometry Pass (Deferred)
   ├─> Render to G-buffer (position, normal, albedo, metallic/roughness)
   └─> Output: Multiple render targets (MRT)

4. Lighting Pass
   ├─> Read G-buffer
   ├─> Apply directional lights + shadows
   ├─> Apply point lights + shadow cubemaps
   └─> Output: HDR color buffer

5. Skybox Pass
   └─> Render skybox to background

6. Post-Processing
   ├─> Bloom (bright pass → Gaussian blur → combine)
   ├─> Tone mapping (HDR → LDR)
   └─> Gamma correction

7. FXAA Pass
   └─> Anti-aliasing

8. Debug Rendering
   └─> Lines, wireframes, bounding boxes

9. Swap Buffers
   └─> Present to screen
```

---

## OpenGL Patterns

### Current Patterns (Pre-DSA)

#### Buffer Creation
```cpp
glGenBuffers(1, &handle);
glBindBuffer(target, handle);
glBufferData(target, size, data, usage);
```

#### Texture Creation
```cpp
glGenTextures(1, &handle);
glBindTexture(target, handle);
glTexImage2D(target, level, internalFormat, width, height, 0, format, type, data);
glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
glGenerateMipmap(target);
```

#### Framebuffer Creation
```cpp
glGenFramebuffers(1, &handle);
glBindFramebuffer(GL_FRAMEBUFFER, handle);
glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, target, texture, level);
glDrawBuffers(count, attachments);
```

#### Vertex Array Configuration
```cpp
glBindVertexArray(vao);
glBindBuffer(GL_ARRAY_BUFFER, vbo);
glEnableVertexAttribArray(index);
glVertexAttribPointer(index, size, type, normalized, stride, offset);
```

### State Management Issues
- **Global state pollution** - Bindings affect all subsequent calls
- **Thread safety concerns** - Dual-context architecture requires careful synchronization
- **Performance overhead** - Redundant state changes

**Solution:** DSA conversion (see DSA_CONVERSION_PLAN.md)

---

## Dependencies

### External Libraries
| Library | Purpose | Version |
|---------|---------|---------|
| GLFW | Window/context creation | 3.3+ |
| GLAD/GLEW | OpenGL loader | GL 4.3+ |
| GLM | Math library (vectors, matrices) | 0.9.9+ |
| Assimp | 3D model loading | 5.0+ |
| stb_image | Image loading (PNG, JPG, TGA) | Single-header |

### Internal Dependencies
- `Graphics` library depends on:
  - GLFW (windowing)
  - Math utilities (GLM wrappers)
  - File I/O (resource loading)

---

## Usage Examples

### Basic Rendering Setup

```cpp
// 1. Create window and OpenGL context (via GLFW)
GLFWwindow* window = glfwCreateWindow(1920, 1080, "Game", nullptr, nullptr);
glfwMakeContextCurrent(window);

// 2. Initialize GLAD
gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

// 3. Create scene renderer
SceneRenderer renderer;
renderer.Initialize();

// 4. Load resources
auto shader = std::make_shared<Shader>("pbr.vert", "pbr.frag");
auto model = std::make_shared<Model>("assets/models/character.fbx");

// 5. Setup scene
Scene scene;
scene.AddEntity(model);

Camera camera;
camera.SetPerspective(45.0f, 1920.0f / 1080.0f, 0.1f, 100.0f);
camera.SetPosition(glm::vec3(0, 2, 5));

// 6. Render loop
while (!glfwWindowShouldClose(window)) {
    // Update
    scene.Update(deltaTime);
    camera.Update(deltaTime);

    // Render
    renderer.RenderScene(scene, camera);

    // Swap
    glfwSwapBuffers(window);
    glfwPollEvents();
}
```

### Creating Custom Geometry

```cpp
// Define vertices
std::vector<Vertex> vertices = {
    { glm::vec3(-1, -1, 0), glm::vec3(0, 0, 1), glm::vec2(0, 0) },
    { glm::vec3( 1, -1, 0), glm::vec3(0, 0, 1), glm::vec2(1, 0) },
    { glm::vec3( 0,  1, 0), glm::vec3(0, 0, 1), glm::vec2(0.5, 1) }
};

std::vector<uint32_t> indices = { 0, 1, 2 };

// Create mesh
Mesh triangle(vertices, indices, {});

// Draw
triangle.Draw(shader);
```

### Material System

```cpp
// Load textures
unsigned int albedoMap = TextureLoader::CreateGPUTexture(TextureLoader::LoadTextureFromFile("albedo.png"), false);
unsigned int normalMap = TextureLoader::CreateGPUTexture(TextureLoader::LoadTextureFromFile("normal.png"), false);

// Create material
auto pbrShader = std::make_shared<Shader>("pbr.vert", "pbr.frag");
Material material(pbrShader);
material.SetTexture("u_AlbedoMap", albedoMap);
material.SetTexture("u_NormalMap", normalMap);
material.SetFloat("u_Metallic", 0.5f);
material.SetFloat("u_Roughness", 0.3f);

// Create instance with overrides
MaterialInstance instance(material);
instance.SetFloat("u_Roughness", 0.8f);  // Override roughness

// Draw with material
instance.Bind();
mesh.Draw(pbrShader);
```

### Framebuffer Rendering

```cpp
// Create FBO spec
FramebufferSpecification spec;
spec.Width = 1920;
spec.Height = 1080;
spec.Samples = 4;  // 4x MSAA
spec.Attachments = {
    FramebufferTextureFormat::RGBA16F,  // HDR color
    FramebufferTextureFormat::Depth24Stencil8
};

// Create FBO
FrameBuffer fbo(spec);

// Render to FBO
fbo.Bind();
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
// ... render scene ...
fbo.Unbind();

// Use FBO color attachment as texture
unsigned int colorTexture = fbo.GetColorAttachmentRendererID(0);
glBindTextureUnit(0, colorTexture);
```

---

## Performance Considerations

### Current Bottlenecks
1. **State changes** - Excessive glBind* calls (target for DSA conversion)
2. **Draw call count** - Use instancing where possible
3. **Shader switching** - Minimize material changes (sort by material)
4. **Texture binding** - Use bindless textures (GL 4.5+) or texture arrays

### Optimization Strategies
- **Instanced rendering** - SSBOs for transform data (currently implemented)
- **Frustum culling** - Reduce draw calls for off-screen objects
- **LOD system** - Distance-based mesh detail
- **Texture atlases** - Reduce texture switches
- **UBO sharing** - Camera/light data in UBOs (currently implemented)

---

## Multi-Threading Architecture

### Dual Context Design
```
Editor Thread (Context 1)
  ├─> UI rendering
  ├─> Scene editing
  └─> Asset loading

Engine Thread (Context 2)
  ├─> Game simulation
  ├─> Main rendering
  └─> Physics updates

Shared Resources:
  ├─> Textures (via glShareLists or shared context)
  ├─> Buffers
  └─> Shader programs
```

### Synchronization
- Context switching via `glfwMakeContextCurrent()`
- Shared resources created on main thread
- Command buffers for cross-thread rendering

---

## Known Issues & Future Work

### Current Limitations
- ❌ Not using DSA (see DSA_CONVERSION_PLAN.md)
- ❌ Limited texture compression support (DDS only)
- ❌ No bindless textures
- ❌ Manual texture slot management

### Planned Improvements
1. **DSA Conversion** - Eliminate bind-to-edit patterns (in progress)
2. **Bindless Textures** - GL_ARB_bindless_texture for texture arrays
3. **Multi-draw Indirect** - Reduce draw call overhead
4. **Compute Shaders** - GPU-based culling, particle systems
5. **Sparse Textures** - Virtual texturing for large worlds

---

## Quick Reference

### Common File Locations
| What | Where |
|------|-------|
| Buffer wrappers | `include/Buffer/*.h` |
| Texture loading | `src/Resources/Texture.cpp` |
| Shader compilation | `src/Resources/Shader.cpp` |
| Main renderer | `src/Renderer/SceneRenderer.cpp` |
| Shadow mapping | `src/RenderPasses/ShadowMappingPass.cpp` |
| Deferred lighting | `src/RenderPasses/LightingPass.cpp` |
| Post-processing | `src/RenderPasses/PostProcessingPass.cpp` |

### Key Line Numbers (for DSA conversion)
| File | Lines | What |
|------|-------|------|
| `VertexBuffer.cpp` | 21-48 | Buffer creation |
| `IndexBuffer.cpp` | 21-42 | IBO creation |
| `UniformBuffer.cpp` | 21-49 | UBO creation |
| `ShaderStorageBuffer.cpp` | 19-196 | SSBO operations |
| `VertexArray.cpp` | 54-105 | Attribute binding |
| `FrameBuffer.cpp` | 90-280 | FBO invalidation |
| `Texture.cpp` | 84-124 | 2D texture creation |
| `Texture.cpp` | 206-277 | Cubemap creation |
| `TextureSlotManager.cpp` | 72-94 | Texture binding |

---

## Glossary

| Term | Meaning |
|------|---------|
| **VAO** | Vertex Array Object - stores vertex attribute configuration |
| **VBO** | Vertex Buffer Object - stores vertex data |
| **IBO/EBO** | Index Buffer Object / Element Buffer Object - stores indices |
| **UBO** | Uniform Buffer Object - shared shader uniforms |
| **SSBO** | Shader Storage Buffer Object - large data arrays (compute/instancing) |
| **FBO** | Framebuffer Object - off-screen render target |
| **MRT** | Multiple Render Targets - render to multiple textures simultaneously |
| **G-buffer** | Geometry buffer - intermediate textures in deferred rendering |
| **DSA** | Direct State Access - modern OpenGL pattern (GL 4.5+) |
| **PBR** | Physically Based Rendering - realistic material shading |
| **HDR** | High Dynamic Range - lighting values beyond 0-1 |
| **MSAA** | Multisample Anti-Aliasing - hardware anti-aliasing |

---

**For DSA conversion details, see:** `DSA_CONVERSION_PLAN.md`
**For implementation questions, refer to source files directly**

**Last Updated:** March 12, 2026
