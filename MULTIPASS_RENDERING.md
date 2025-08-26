# Multi-Pass Rendering with the Graphics Pipeline

## Overview

The pipeline system is specifically designed for **multi-pass rendering**, enabling various modern rendering techniques through a flexible pass-based architecture.

## Core Architecture

The pipeline system is built on 3 core classes:

1. **`RenderPipeline`** - Container that manages multiple render passes
2. **`RenderPass`** - Individual rendering stage with framebuffer + render function  
3. **`PostProcessStack`** - Post-processing effects chain

## Current Implementation - Deferred Rendering Pipeline

```cpp
void SceneRenderer::InitializePipeline() {
    // PASS 1: Geometry Pass (G-Buffer Generation)
    auto geometryPass = std::make_shared<RenderPass>("GeometryPass", FBOSpecs{
        1280, 720,
        {
            { FBOTextureFormat::RGBA8 },          // Color buffer
            { FBOTextureFormat::RGBA8 },          // Normal buffer  
            { FBOTextureFormat::RED_INTEGER },    // Entity ID (for picking)
            { FBOTextureFormat::DEPTH24STENCIL8 } // Depth buffer
        }
    });
    
    geometryPass->SetRenderFunction([this]() {
        // Render all geometry to G-Buffer
        m_MeshRenderer->Render(m_Scene.get(), *m_Camera);
    });
    
    m_Pipeline->AddPass(geometryPass);
}
```

## Common Multi-Pass Rendering Scenarios

### 1. Shadow Mapping (2+ passes)

```cpp
// PASS 1: Shadow depth pass
auto shadowPass = std::make_shared<RenderPass>("ShadowPass", FBOSpecs{
    2048, 2048,  // Shadow map resolution
    {{ FBOTextureFormat::DEPTH24STENCIL8 }}  // Only depth
});

shadowPass->SetRenderFunction([this]() {
    // Render from light's perspective
    m_MeshRenderer->RenderDepthOnly(m_Scene.get(), *m_LightCamera);
});

// PASS 2: Main geometry pass with shadows
auto mainPass = std::make_shared<RenderPass>("MainPass", mainFBOSpecs);
mainPass->SetRenderFunction([this]() {
    // Bind shadow map from pass 1
    auto shadowMap = m_Pipeline->GetPass("ShadowPass")->GetFramebuffer();
    glBindTexture(GL_TEXTURE_2D, shadowMap->GetColorAttachmentRendererID());
    
    // Render with shadow sampling
    m_MeshRenderer->RenderWithShadows(m_Scene.get(), *m_Camera);
});
```

### 2. Deferred Lighting (3 passes)

```cpp
// PASS 1: Geometry Pass (G-Buffer)
auto geometryPass = std::make_shared<RenderPass>("Geometry", gBufferSpecs);

// PASS 2: Lighting Pass (using G-Buffer)
auto lightingPass = std::make_shared<RenderPass>("Lighting", FBOSpecs{
    1280, 720,
    {{ FBOTextureFormat::RGBA8 }}  // Final lit color
});

lightingPass->SetRenderFunction([this]() {
    // Get G-Buffer textures from geometry pass
    auto gBuffer = m_Pipeline->GetPass("Geometry")->GetFramebuffer();
    
    // Bind G-Buffer textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer->GetColorAttachmentRendererID(0));  // Albedo
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer->GetColorAttachmentRendererID(1));  // Normals
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer->GetColorAttachmentRendererID(3));  // Depth
    
    // Render fullscreen quad with lighting calculations
    m_LightingShader->use();
    DrawFullscreenQuad();
});

// PASS 3: Post-Processing
auto postProcessPass = std::make_shared<RenderPass>("PostProcess", screenSpecs);
```

### 3. Transparency Handling (2 passes)

```cpp
// PASS 1: Opaque objects
auto opaquePass = std::make_shared<RenderPass>("Opaque", mainFBOSpecs);
opaquePass->SetRenderFunction([this]() {
    m_MeshRenderer->RenderOpaque(m_Scene.get(), *m_Camera);
});

// PASS 2: Transparent objects (sorted back-to-front)
auto transparentPass = std::make_shared<RenderPass>("Transparent", mainFBOSpecs);
transparentPass->SetRenderFunction([this]() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    m_MeshRenderer->RenderTransparent(m_Scene.get(), *m_Camera);
    
    glDisable(GL_BLEND);
});
```

### 4. Screen-Space Effects (multiple passes)

```cpp
// PASS 1: Render scene
auto scenePass = std::make_shared<RenderPass>("Scene", sceneFBOSpecs);

// PASS 2: SSAO (Screen Space Ambient Occlusion)
auto ssaoPass = std::make_shared<RenderPass>("SSAO", ssaoFBOSpecs);
ssaoPass->SetRenderFunction([this]() {
    auto sceneDepth = m_Pipeline->GetPass("Scene")->GetFramebuffer();
    // Use depth + normals to calculate ambient occlusion
});

// PASS 3: Bloom extraction
auto bloomPass = std::make_shared<RenderPass>("Bloom", bloomFBOSpecs);
bloomPass->SetRenderFunction([this]() {
    auto sceneColor = m_Pipeline->GetPass("Scene")->GetFramebuffer();
    // Extract bright pixels for bloom
});

// PASS 4: Final composite
auto compositePass = std::make_shared<RenderPass>("Composite", screenSpecs);
```

## Key Benefits of Multi-Pass Architecture

### 1. Pass Dependencies

```cpp
// Later passes can access earlier pass results
auto previousPass = m_Pipeline->GetPass("GeometryPass");
auto gBufferTexture = previousPass->GetFramebuffer()->GetColorAttachmentRendererID();
```

### 2. Flexible Pass Ordering

```cpp
// Can dynamically reorder or insert passes
m_Pipeline->RemovePass("OldPass");
m_Pipeline->AddPass(newPass);  // Adds to end
```

### 3. Conditional Passes

```cpp
geometryPass->SetRenderFunction([this]() {
    if (m_EnableShadows) {
        // Shadow pass was already executed
        BindShadowMap();
    }
    m_MeshRenderer->Render(m_Scene.get(), *m_Camera);
});
```

### 4. Pass Reuse

```cpp
// Can execute same pass multiple times (e.g., cubemap faces)
for (int face = 0; face < 6; face++) {
    envMapPass->SetRenderFunction([this, face]() {
        SetupCubemapFace(face);
        m_MeshRenderer->Render(m_Scene.get(), *m_CubemapCameras[face]);
    });
    envMapPass->Execute();
}
```

## Advanced Multi-Pass Techniques

### Temporal Effects (using previous frame)

```cpp
// Motion blur, TAA, temporal upsampling
auto currentFrame = m_Pipeline->GetPass("Current");
auto previousFrame = m_Pipeline->GetPass("Previous");
// Blend or analyze differences
```

### Multi-resolution Rendering

```cpp
// Low-res pass for expensive effects
auto lowResPass = std::make_shared<RenderPass>("LowRes", FBOSpecs{640, 360, ...});
// Full-res pass with upsampling
auto fullResPass = std::make_shared<RenderPass>("FullRes", FBOSpecs{1280, 720, ...});
```

### Hierarchical Depth Buffers

```cpp
// Multiple depth passes at different resolutions for culling
auto hiZPass1 = std::make_shared<RenderPass>("HiZ_512", ...);
auto hiZPass2 = std::make_shared<RenderPass>("HiZ_256", ...);
auto hiZPass3 = std::make_shared<RenderPass>("HiZ_128", ...);
```

## Pipeline Execution Flow

```
Application::RenderFrame()
├── SceneRenderer::Render()
│   └── RenderPipeline::Execute()
│       ├── ShadowPass::Execute()
│       │   ├── FrameBuffer::Bind() (Shadow Map)
│       │   ├── RenderFunction() [Shadow Rendering]
│       │   └── FrameBuffer::Unbind()
│       ├── GeometryPass::Execute()
│       │   ├── FrameBuffer::Bind() (G-Buffer)
│       │   ├── RenderFunction() [Geometry Rendering]
│       │   └── FrameBuffer::Unbind()
│       ├── LightingPass::Execute()
│       │   ├── FrameBuffer::Bind() (Light Buffer)
│       │   ├── RenderFunction() [Deferred Lighting]
│       │   └── FrameBuffer::Unbind()
│       └── PostProcessPass::Execute()
│           ├── FrameBuffer::Bind() (Screen)
│           ├── RenderFunction() [Post Effects]
│           └── FrameBuffer::Unbind()
└── GraphicsContext::SwapBuffers()
```

## Summary

The pipeline system is **built for multi-pass rendering** and provides:

1. **Pass isolation** - Each pass has its own framebuffer
2. **Pass communication** - Later passes can read earlier pass outputs
3. **Flexible ordering** - Easy to add/remove/reorder passes
4. **Resource management** - Automatic framebuffer binding/unbinding
5. **Performance optimization** - Command buffer sorting happens per pass

This architecture enables modern rendering techniques like:
- Deferred shading
- Shadow mapping
- Post-processing chains
- Screen-space effects
- Temporal effects
- Multi-resolution rendering

All of these techniques require multiple rendering passes with different targets and shaders, which the pipeline system handles elegantly through its pass-based architecture.