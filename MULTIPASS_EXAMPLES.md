# Multi-Pass Rendering Examples

## Single Pipeline with Multiple Passes

### Example 1: Deferred Rendering Pipeline (Multiple Passes)

```cpp
class DeferredPipeline {
    std::unique_ptr<RenderPipeline> CreatePipeline() {
        auto pipeline = std::make_unique<RenderPipeline>("Deferred");
        
        // PASS 1: G-Buffer Generation
        auto gBufferPass = std::make_shared<RenderPass>("GBuffer", FBOSpecs{
            1920, 1080,
            {
                { FBOTextureFormat::RGBA8 },      // Albedo + Metallic
                { FBOTextureFormat::RGBA16F },    // Normal + Roughness  
                { FBOTextureFormat::RGBA8 },      // Motion + AO + MaterialID
                { FBOTextureFormat::DEPTH24STENCIL8 }
            }
        });
        
        gBufferPass->SetRenderFunction([this]() {
            // Render all opaque geometry to G-Buffer
            // Uses MeshRenderer + InstancedRenderer with deferred shaders
        });
        
        // PASS 2: Lighting Pass
        auto lightingPass = std::make_shared<RenderPass>("Lighting", FBOSpecs{
            1920, 1080,
            { { FBOTextureFormat::RGBA16F } }  // HDR lighting result
        });
        
        lightingPass->SetRenderFunction([this, gBufferPass]() {
            // Fullscreen quad using G-Buffer textures
            // Calculate lighting for all pixels
            auto gBuffer = gBufferPass->GetFramebuffer();
            // Bind G-Buffer textures as input
            // Render fullscreen quad with lighting shader
        });
        
        // PASS 3: Transparent Pass  
        auto transparentPass = std::make_shared<RenderPass>("Transparent", FBOSpecs{
            1920, 1080,
            { { FBOTextureFormat::RGBA16F } }
        });
        
        transparentPass->SetRenderFunction([this, lightingPass]() {
            // Forward render transparent objects on top of lighting result
            // Enable blending, render transparent geometry
        });
        
        // PASS 4: Sky/Environment Pass
        auto skyPass = std::make_shared<RenderPass>("Sky", FBOSpecs{
            1920, 1080,
            { { FBOTextureFormat::RGBA16F } }
        });
        
        skyPass->SetRenderFunction([this]() {
            // Render skybox/environment mapping
            // Only render where depth = far plane
        });
        
        // Add all passes to pipeline
        pipeline->AddPass(gBufferPass);     // Pass 1
        pipeline->AddPass(lightingPass);    // Pass 2  
        pipeline->AddPass(transparentPass); // Pass 3
        pipeline->AddPass(skyPass);         // Pass 4
        
        return pipeline;
    }
};
```

### Example 2: Advanced Shadow Mapping Pipeline (Multiple Passes)

```cpp
class AdvancedShadowPipeline {
    std::unique_ptr<RenderPipeline> CreatePipeline() {
        auto pipeline = std::make_unique<RenderPipeline>("AdvancedShadows");
        
        // PASS 1: Cascaded Shadow Map Generation
        for (int cascade = 0; cascade < 4; ++cascade) {
            auto cascadePass = std::make_shared<RenderPass>(
                "ShadowCascade" + std::to_string(cascade),
                FBOSpecs{
                    2048, 2048,
                    { { FBOTextureFormat::DEPTH24STENCIL8 } }
                }
            );
            
            cascadePass->SetRenderFunction([this, cascade]() {
                // Setup light camera for this cascade distance
                SetupCascadeCamera(cascade);
                // Render scene from light POV for this cascade
                RenderShadowCasters();
            });
            
            pipeline->AddPass(cascadePass);
        }
        
        // PASS 2: Shadow Map Filtering (Optional)
        auto filterPass = std::make_shared<RenderPass>("ShadowFilter", FBOSpecs{
            2048, 2048,
            { { FBOTextureFormat::R16F } }  // Filtered shadow map
        });
        
        filterPass->SetRenderFunction([this]() {
            // Apply PCF, VSM, or other shadow filtering
            ApplyShadowFiltering();
        });
        
        pipeline->AddPass(filterPass);
        
        return pipeline;
    }
};
```

### Example 3: Post-Processing Pipeline (Multiple Passes)

```cpp
class AdvancedPostProcessPipeline {
    std::unique_ptr<RenderPipeline> CreatePipeline() {
        auto pipeline = std::make_unique<RenderPipeline>("PostProcessing");
        
        // PASS 1: Bright Pass (Extract bright pixels for bloom)
        auto brightPass = std::make_shared<RenderPass>("BrightPass", FBOSpecs{
            960, 540,  // Half resolution
            { { FBOTextureFormat::RGBA16F } }
        });
        
        // PASS 2: Horizontal Blur
        auto hBlurPass = std::make_shared<RenderPass>("HorizontalBlur", FBOSpecs{
            960, 540,
            { { FBOTextureFormat::RGBA16F } }
        });
        
        // PASS 3: Vertical Blur  
        auto vBlurPass = std::make_shared<RenderPass>("VerticalBlur", FBOSpecs{
            960, 540,
            { { FBOTextureFormat::RGBA16F } }
        });
        
        // PASS 4: Tone Mapping + Bloom Combine
        auto toneMappingPass = std::make_shared<RenderPass>("ToneMapping", FBOSpecs{
            1920, 1080,
            { { FBOTextureFormat::RGBA8 } }  // Final LDR output
        });
        
        toneMappingPass->SetRenderFunction([brightPass, vBlurPass]() {
            // Bind original HDR scene
            // Bind blurred bloom texture  
            // Apply tone mapping + bloom combination
        });
        
        // PASS 5: FXAA Anti-Aliasing
        auto fxaaPass = std::make_shared<RenderPass>("FXAA", FBOSpecs{
            1920, 1080,
            { { FBOTextureFormat::RGBA8 } }
        });
        
        pipeline->AddPass(brightPass);
        pipeline->AddPass(hBlurPass);
        pipeline->AddPass(vBlurPass);
        pipeline->AddPass(toneMappingPass);
        pipeline->AddPass(fxaaPass);
        
        return pipeline;
    }
};
```

## Multiple Independent Pipelines

### Example: Complete Rendering System

```cpp
void SceneRenderer::InitializeAdvancedPipelines() {
    // PIPELINE 1: Shadow Mapping (4 passes for cascades)
    auto shadowPipeline = m_ShadowCreator->CreatePipeline();
    AddPipeline("ShadowMapping", std::move(shadowPipeline));
    
    // PIPELINE 2: Environment Mapping (3 passes for cubemap)
    auto envPipeline = std::make_unique<RenderPipeline>("Environment");
    // ... setup environment cube map generation passes
    AddPipeline("Environment", std::move(envPipeline));
    
    // PIPELINE 3: Main Deferred Rendering (4 passes)
    auto deferredPipeline = m_DeferredCreator->CreatePipeline();
    AddPipeline("Deferred", std::move(deferredPipeline));
    
    // PIPELINE 4: Screen Space Effects (2 passes)
    auto ssrPipeline = std::make_unique<RenderPipeline>("SSR");
    // ... setup screen space reflections
    AddPipeline("SSR", std::move(ssrPipeline));
    
    // PIPELINE 5: Post Processing (5 passes)  
    auto postPipeline = m_PostProcessCreator->CreatePipeline();
    AddPipeline("PostProcessing", std::move(postPipeline));
    
    // PIPELINE 6: UI Rendering (1 pass)
    auto uiPipeline = std::make_unique<RenderPipeline>("UI");
    // ... setup UI rendering
    AddPipeline("UI", std::move(uiPipeline));
    
    // Set execution order
    SetPipelineOrder({
        "Environment",    // Update environment map (maybe not every frame)
        "ShadowMapping",  // Generate shadow maps
        "Deferred",       // Main scene rendering (4 passes)
        "SSR",           // Screen space reflections
        "PostProcessing", // Post-processing effects (5 passes) 
        "UI"             // UI overlay
    });
    
    // Configure update frequencies
    EnablePipeline("Environment", false);  // Update manually when needed
    m_EnvironmentUpdateCounter = 0;
}

void SceneRenderer::Render() {
    // Update environment map every 60 frames (1 second at 60fps)
    if (++m_EnvironmentUpdateCounter >= 60) {
        EnablePipeline("Environment", true);
        m_EnvironmentUpdateCounter = 0;
    } else {
        EnablePipeline("Environment", false);
    }
    
    // Execute all enabled pipelines
    // ... existing render code
}
```

## Performance Considerations

### Frame Time Breakdown (Advanced Setup):
```
Total Frame Time: 16.67ms (60 FPS)
├── Environment: 0.0ms   (disabled most frames)
├── ShadowMapping: 2.0ms (4 cascade passes)
├── Deferred: 8.0ms      (4 passes: GBuffer + Lighting + Transparent + Sky)
├── SSR: 1.5ms           (2 passes: trace + denoise)
├── PostProcessing: 4.0ms (5 passes: bright + blur + blur + tone + fxaa)
└── UI: 1.17ms           (1 pass)
```

## Dynamic Pipeline Switching

```cpp
// Switch rendering quality based on performance
void SceneRenderer::SetRenderingQuality(QualityLevel quality) {
    switch (quality) {
        case QualityLevel::Low:
            // Forward rendering, no post-processing
            SetPipelineOrder({"ShadowMapping", "MainRendering"});
            EnablePipeline("Deferred", false);
            EnablePipeline("PostProcessing", false);
            break;
            
        case QualityLevel::Medium:  
            // Deferred rendering, basic post-processing
            SetPipelineOrder({"ShadowMapping", "Deferred", "PostProcessing"});
            m_PostProcessCreator->EnableBloom(false);
            m_PostProcessCreator->EnableFXAA(true);
            break;
            
        case QualityLevel::High:
            // Full pipeline with all effects
            SetPipelineOrder({
                "Environment", "ShadowMapping", "Deferred", 
                "SSR", "PostProcessing", "UI"
            });
            m_PostProcessCreator->EnableBloom(true);
            m_PostProcessCreator->EnableFXAA(true);
            break;
    }
}
```