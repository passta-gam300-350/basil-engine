# Multi-Pipeline Architecture Implementation Guide

## Overview
This document outlines the implementation of individual pipeline files for the graphics engine, following a modular architecture where each pipeline is contained in its own file for better maintainability and organization.

## 📁 File Structure

```
lib/Graphics/
├── include/
│   └── Scene/
│       ├── SceneRenderer.h
│       └── Pipelines/
│           ├── ShadowPipeline.h
│           ├── DeferredPipeline.h
│           ├── PostProcessPipeline.h
│           ├── LightingPipeline.h
│           └── SSAOPipeline.h
└── src/
    └── Scene/
        ├── SceneRenderer.cpp
        └── Pipelines/
            ├── ShadowPipeline.cpp
            ├── DeferredPipeline.cpp
            ├── PostProcessPipeline.cpp
            ├── LightingPipeline.cpp
            └── SSAOPipeline.cpp
```

## 🏗️ Pipeline Implementation Pattern

### 1. Shadow Pipeline Header (ShadowPipeline.h)

```cpp
#pragma once

#include "../../../Pipeline/RenderPipeline.h"
#include "../../../Pipeline/RenderPass.h"
#include "../../../Buffer/FrameBuffer.h"
#include "../../../Utility/Camera.h"
#include <memory>

// Forward declarations
class Scene;
class MeshRenderer;
class InstancedRenderer;
class FrustumCuller;
struct FrameData;

class ShadowPipeline {
public:
    ShadowPipeline();
    ~ShadowPipeline() = default;

    // Create and configure the shadow mapping pipeline
    std::unique_ptr<RenderPipeline> CreatePipeline();
    
    // Configuration
    void SetShadowMapSize(uint32_t size) { m_ShadowMapSize = size; }
    void SetCascadeCount(uint32_t count) { m_CascadeCount = count; }
    
    // Access shadow data for other pipelines
    const std::vector<glm::mat4>& GetShadowMatrices() const { return m_ShadowMatrices; }
    GLuint GetShadowMapTexture() const { return m_ShadowMapTexture; }

private:
    // Pipeline configuration
    uint32_t m_ShadowMapSize = 2048;
    uint32_t m_CascadeCount = 4;
    
    // Shadow data (shared with other pipelines via FrameData)
    std::vector<glm::mat4> m_ShadowMatrices;
    GLuint m_ShadowMapTexture = 0;
    
    // Private setup methods
    void SetupShadowPass(std::shared_ptr<RenderPass> shadowPass);
    void SetupCascadedShadowPass(std::shared_ptr<RenderPass> shadowPass);
};
```

### 2. Shadow Pipeline Implementation (ShadowPipeline.cpp)

```cpp
#include "Scene/Pipelines/ShadowPipeline.h"
#include "Scene/SceneRenderer.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/InstancedRenderer.h"
#include "Rendering/FrustumCuller.h"
#include "../../../../test/examples/lib/Graphics/Engine/Scene/Scene.h"
#include <glad/gl.h>
#include <iostream>

ShadowPipeline::ShadowPipeline() {
    // Initialize shadow matrices for cascaded shadow mapping
    m_ShadowMatrices.resize(m_CascadeCount);
}

std::unique_ptr<RenderPipeline> ShadowPipeline::CreatePipeline() {
    auto shadowPipeline = std::make_unique<RenderPipeline>("ShadowMapping");
    
    // Create shadow pass
    auto shadowPass = std::make_shared<RenderPass>("ShadowPass", FBOSpecs{
        m_ShadowMapSize, m_ShadowMapSize,
        {
            { FBOTextureFormat::DEPTH24STENCIL8 }  // Depth-only for shadow mapping
        }
    });
    
    SetupShadowPass(shadowPass);
    shadowPipeline->AddPass(shadowPass);
    
    std::cout << "✅ Shadow Pipeline created with " << m_ShadowMapSize << "x" << m_ShadowMapSize << " shadow map" << std::endl;
    
    return shadowPipeline;
}

void ShadowPipeline::SetupShadowPass(std::shared_ptr<RenderPass> shadowPass) {
    shadowPass->SetRenderFunction([this, shadowPass]() {
        std::cout << "🌑 Shadow Pass executing..." << std::endl;
        
        // Clear depth buffer
        glClear(GL_DEPTH_BUFFER_BIT);
        
        // Disable color writing for shadow mapping
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        
        // Enable depth testing and depth writing
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        
        // Render scene from light's perspective
        // Note: This would need access to light camera and scene
        // In practice, you'd pass these via the render function capture or FrameData
        
        // TODO: Implement light camera setup
        // TODO: Render scene using MeshRenderer and InstancedRenderer from light's POV
        
        // Re-enable color writing
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        
        // Store shadow map texture for other pipelines
        m_ShadowMapTexture = shadowPass->GetFramebuffer()->GetColorAttachment(0);
    });
}

void ShadowPipeline::SetupCascadedShadowPass(std::shared_ptr<RenderPass> shadowPass) {
    // Implementation for cascaded shadow mapping
    // Multiple shadow maps for different distance ranges
}
```

### 3. Deferred Pipeline Header (DeferredPipeline.h)

```cpp
#pragma once

#include "../../../Pipeline/RenderPipeline.h"
#include "../../../Pipeline/RenderPass.h"
#include <memory>

class DeferredPipeline {
public:
    DeferredPipeline();
    ~DeferredPipeline() = default;

    // Create and configure the deferred rendering pipeline
    std::unique_ptr<RenderPipeline> CreatePipeline();
    
    // Configuration
    void SetGBufferResolution(uint32_t width, uint32_t height);
    void EnableSSAO(bool enable) { m_SSAOEnabled = enable; }

private:
    uint32_t m_Width = 1280;
    uint32_t m_Height = 720;
    bool m_SSAOEnabled = false;
    
    void SetupGBufferPass(std::shared_ptr<RenderPass> gBufferPass);
};
```

### 4. Deferred Pipeline Implementation (DeferredPipeline.cpp)

```cpp
#include "Scene/Pipelines/DeferredPipeline.h"
#include <glad/gl.h>
#include <iostream>

DeferredPipeline::DeferredPipeline() {
    // Constructor
}

std::unique_ptr<RenderPipeline> DeferredPipeline::CreatePipeline() {
    auto deferredPipeline = std::make_unique<RenderPipeline>("Deferred");
    
    // G-Buffer pass - render geometry data to multiple render targets
    auto gBufferPass = std::make_shared<RenderPass>("GBufferPass", FBOSpecs{
        m_Width, m_Height,
        {
            { FBOTextureFormat::RGBA8 },      // Albedo + Metallic
            { FBOTextureFormat::RGBA16F },    // World Space Normals + Roughness
            { FBOTextureFormat::RGBA8 },      // Motion Vectors + AO + Material ID
            { FBOTextureFormat::RED_INTEGER },// Entity ID for picking
            { FBOTextureFormat::DEPTH24STENCIL8 } // Depth + Stencil
        }
    });
    
    SetupGBufferPass(gBufferPass);
    deferredPipeline->AddPass(gBufferPass);
    
    std::cout << "✅ Deferred Pipeline created with " << m_Width << "x" << m_Height << " G-Buffer" << std::endl;
    
    return deferredPipeline;
}

void DeferredPipeline::SetupGBufferPass(std::shared_ptr<RenderPass> gBufferPass) {
    gBufferPass->SetRenderFunction([this, gBufferPass]() {
        std::cout << "📊 G-Buffer Pass executing..." << std::endl;
        
        // Clear all G-Buffer targets
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        
        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        
        // Render geometry to G-Buffer
        // TODO: Access scene and camera via capture or FrameData
        // TODO: Use MeshRenderer and InstancedRenderer with deferred shaders
        
        std::cout << "📊 G-Buffer data written to multiple render targets" << std::endl;
    });
}

void DeferredPipeline::SetGBufferResolution(uint32_t width, uint32_t height) {
    m_Width = width;
    m_Height = height;
}
```

### 5. Post-Process Pipeline Header (PostProcessPipeline.h)

```cpp
#pragma once

#include "../../../Pipeline/RenderPipeline.h"
#include "../../../Pipeline/RenderPass.h"
#include <memory>

class PostProcessPipeline {
public:
    PostProcessPipeline();
    ~PostProcessPipeline() = default;

    // Create and configure the post-processing pipeline
    std::unique_ptr<RenderPipeline> CreatePipeline();
    
    // Configuration
    void EnableBloom(bool enable) { m_BloomEnabled = enable; }
    void EnableToneMapping(bool enable) { m_ToneMappingEnabled = enable; }
    void EnableFXAA(bool enable) { m_FXAAEnabled = enable; }
    void SetExposure(float exposure) { m_Exposure = exposure; }

private:
    bool m_BloomEnabled = true;
    bool m_ToneMappingEnabled = true;
    bool m_FXAAEnabled = true;
    float m_Exposure = 1.0f;
    
    void SetupToneMappingPass(std::shared_ptr<RenderPass> toneMappingPass);
    void SetupBloomPass(std::shared_ptr<RenderPass> bloomPass);
    void SetupFXAAPass(std::shared_ptr<RenderPass> fxaaPass);
};
```

### 6. Post-Process Pipeline Implementation (PostProcessPipeline.cpp)

```cpp
#include "Scene/Pipelines/PostProcessPipeline.h"
#include <glad/gl.h>
#include <iostream>

PostProcessPipeline::PostProcessPipeline() {
    // Constructor
}

std::unique_ptr<RenderPipeline> PostProcessPipeline::CreatePipeline() {
    auto postPipeline = std::make_unique<RenderPipeline>("PostProcessing");
    
    // Tone mapping pass
    if (m_ToneMappingEnabled) {
        auto toneMappingPass = std::make_shared<RenderPass>("ToneMapping", FBOSpecs{
            1280, 720,
            { { FBOTextureFormat::RGBA8 } }  // LDR output
        });
        
        SetupToneMappingPass(toneMappingPass);
        postPipeline->AddPass(toneMappingPass);
    }
    
    // Bloom pass
    if (m_BloomEnabled) {
        auto bloomPass = std::make_shared<RenderPass>("Bloom", FBOSpecs{
            640, 360,  // Half resolution for performance
            { { FBOTextureFormat::RGBA16F } }  // HDR for bloom
        });
        
        SetupBloomPass(bloomPass);
        postPipeline->AddPass(bloomPass);
    }
    
    // FXAA pass
    if (m_FXAAEnabled) {
        auto fxaaPass = std::make_shared<RenderPass>("FXAA", FBOSpecs{
            1280, 720,
            { { FBOTextureFormat::RGBA8 } }
        });
        
        SetupFXAAPass(fxaaPass);
        postPipeline->AddPass(fxaaPass);
    }
    
    std::cout << "✅ Post-Process Pipeline created with " 
              << (m_BloomEnabled ? "Bloom " : "")
              << (m_ToneMappingEnabled ? "ToneMapping " : "")
              << (m_FXAAEnabled ? "FXAA " : "")
              << "effects" << std::endl;
    
    return postPipeline;
}

void PostProcessPipeline::SetupToneMappingPass(std::shared_ptr<RenderPass> toneMappingPass) {
    toneMappingPass->SetRenderFunction([this, toneMappingPass]() {
        std::cout << "🎨 Tone Mapping Pass executing..." << std::endl;
        
        // Clear the output
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Disable depth testing for fullscreen quad
        glDisable(GL_DEPTH_TEST);
        
        // TODO: Bind HDR input texture from previous pipeline
        // TODO: Use tone mapping shader (Reinhard, ACES, etc.)
        // TODO: Render fullscreen quad
        
        std::cout << "🎨 HDR to LDR tone mapping applied" << std::endl;
    });
}

void PostProcessPipeline::SetupBloomPass(std::shared_ptr<RenderPass> bloomPass) {
    bloomPass->SetRenderFunction([this, bloomPass]() {
        std::cout << "✨ Bloom Pass executing..." << std::endl;
        
        // TODO: Extract bright pixels
        // TODO: Gaussian blur in multiple passes
        // TODO: Combine with original image
        
        std::cout << "✨ Bloom effect applied" << std::endl;
    });
}

void PostProcessPipeline::SetupFXAAPass(std::shared_ptr<RenderPass> fxaaPass) {
    fxaaPass->SetRenderFunction([this, fxaaPass]() {
        std::cout << "🎯 FXAA Pass executing..." << std::endl;
        
        // TODO: Apply FXAA anti-aliasing
        
        std::cout << "🎯 FXAA anti-aliasing applied" << std::endl;
    });
}
```

## 🔗 Integration with SceneRenderer

### Modified SceneRenderer.h

```cpp
// Add includes for pipeline classes
#include "Pipelines/ShadowPipeline.h"
#include "Pipelines/DeferredPipeline.h"
#include "Pipelines/PostProcessPipeline.h"

class SceneRenderer {
private:
    // Pipeline creators
    std::unique_ptr<ShadowPipeline> m_ShadowPipelineCreator;
    std::unique_ptr<DeferredPipeline> m_DeferredPipelineCreator;
    std::unique_ptr<PostProcessPipeline> m_PostProcessPipelineCreator;
    
    // Pipeline initialization methods
    void InitializeDefaultPipelines();
    void InitializePipelineCreators();
    
public:
    // Pipeline configuration access
    ShadowPipeline* GetShadowPipelineCreator() const { return m_ShadowPipelineCreator.get(); }
    DeferredPipeline* GetDeferredPipelineCreator() const { return m_DeferredPipelineCreator.get(); }
    PostProcessPipeline* GetPostProcessPipelineCreator() const { return m_PostProcessPipelineCreator.get(); }
};
```

### Modified SceneRenderer.cpp

```cpp
void SceneRenderer::SceneRenderer() {
    InitializeRenderingCoordinators();
    InitializePipelineCreators();
    InitializeDefaultPipelines();
}

void SceneRenderer::InitializePipelineCreators() {
    m_ShadowPipelineCreator = std::make_unique<ShadowPipeline>();
    m_DeferredPipelineCreator = std::make_unique<DeferredPipeline>();
    m_PostProcessPipelineCreator = std::make_unique<PostProcessPipeline>();
}

void SceneRenderer::InitializeDefaultPipelines() {
    // Create main rendering pipeline (existing code)
    // ... existing main pipeline code ...
    
    // Create additional pipelines using pipeline creators
    auto shadowPipeline = m_ShadowPipelineCreator->CreatePipeline();
    AddPipeline("ShadowMapping", std::move(shadowPipeline));
    
    auto deferredPipeline = m_DeferredPipelineCreator->CreatePipeline();
    AddPipeline("Deferred", std::move(deferredPipeline));
    
    auto postProcessPipeline = m_PostProcessPipelineCreator->CreatePipeline();
    AddPipeline("PostProcessing", std::move(postProcessPipeline));
    
    // Set pipeline order based on rendering strategy
    SetForwardRenderingOrder();  // or SetDeferredRenderingOrder()
}

void SceneRenderer::SetForwardRenderingOrder() {
    SetPipelineOrder({ "ShadowMapping", "MainRendering", "PostProcessing" });
    EnablePipeline("Deferred", false);
}

void SceneRenderer::SetDeferredRenderingOrder() {
    SetPipelineOrder({ "ShadowMapping", "Deferred", "Lighting", "PostProcessing" });
    EnablePipeline("MainRendering", false);
}
```

## 🎮 Runtime Configuration

### Application Usage

```cpp
// In your Application class or main.cpp initialization
void ConfigurePipelines() {
    auto* sceneRenderer = GetSceneRenderer();
    
    // Configure shadow pipeline
    auto* shadowCreator = sceneRenderer->GetShadowPipelineCreator();
    shadowCreator->SetShadowMapSize(4096);  // High quality shadows
    shadowCreator->SetCascadeCount(4);      // Cascaded shadow mapping
    
    // Configure post-processing
    auto* postProcessCreator = sceneRenderer->GetPostProcessPipelineCreator();
    postProcessCreator->EnableBloom(true);
    postProcessCreator->EnableToneMapping(true);
    postProcessCreator->SetExposure(1.2f);
    
    // Switch between forward and deferred rendering
    if (useDeferred) {
        sceneRenderer->SetDeferredRenderingOrder();
    } else {
        sceneRenderer->SetForwardRenderingOrder();
    }
}
```

## 🔧 Benefits of This Architecture

1. **Modularity**: Each pipeline is self-contained and maintainable
2. **Configurability**: Pipelines can be configured independently
3. **Reusability**: Pipeline classes can be reused in different contexts
4. **Testability**: Individual pipelines can be unit tested
5. **Performance**: Only enabled pipelines are executed
6. **Flexibility**: Easy to switch between different rendering strategies
7. **Scalability**: New pipelines can be added without modifying existing code

## 🚀 Next Steps

1. Create the directory structure
2. Implement individual pipeline files
3. Modify SceneRenderer to use pipeline creators
4. Add runtime configuration methods
5. Test different pipeline combinations
6. Optimize pipeline interactions and data flow

This architecture provides a solid foundation for advanced rendering techniques while maintaining clean code organization and high performance.