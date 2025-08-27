# Multi-Pass Architecture Decision

## Problem Identified

During architecture analysis, a critical issue was discovered: the current multi-pass rendering design makes the sophisticated command system largely redundant for most rendering passes.

## Architectural Boundaries Clarification

**Important Note**: After further analysis, we clarified that passes should NOT be submittable as commands. The architecture operates at two distinct levels:

### Level 1: Pass Execution (RenderPipeline)
- **Sequential execution** - passes run in fixed order determined by pipeline structure
- **No command system** - passes execute directly one after another
- **Context management** - each pass sets up its own framebuffer and state

### Level 2: Draw Call Optimization (RenderCommandBuffer) 
- **Within-pass sorting** - commands are collected and sorted within each individual pass
- **Fine-grained batching** - individual draw calls are optimized by material/mesh
- **Pass-scoped** - command buffer is cleared and executed per pass

```cpp
// Correct: Pass execution is sequential, commands sort within passes
void RenderPipeline::Execute() {
    for (auto& pass : m_Passes) {
        pass->Execute(); // Fixed order execution
    }
}

void RenderPass::Execute() {
    Begin(); // Set up pass state
    
    Renderer::Get().BeginPassCommands(); // Clear command buffer for this pass
    if (m_RenderFunction) {
        m_RenderFunction(); // Generates fine-grained commands
    }
    Renderer::Get().ExecutePassCommands(); // Sort and execute this pass's commands
    
    End(); // Clean up pass state
}
```

## Current Architecture Issues

### Command System Overhead Without Benefits

The framework assumes **all rendering goes through the command buffer**:
```cpp
// MeshRenderer submits commands
Renderer::Get().Submit(bindShaderCmd, sortKey);
Renderer::Get().Submit(uniformsCmd, sortKey);
Renderer::Get().Submit(texturesCmd, sortKey);
Renderer::Get().Submit(drawCmd, sortKey);

// Renderer sorts and batches commands
void Renderer::EndFrame() {
    m_CommandBuffer.Sort();    // Expensive sorting
    m_CommandBuffer.Execute(); // Batch execution
}
```

### Multi-Pass Reality

True multi-pass rendering often does **direct OpenGL calls**:

```cpp
// Lighting pass - no commands needed
lightingPass->SetRenderFunction([this]() {
    // Direct OpenGL - bypasses entire command system
    glUseProgram(lightingShader->ID);
    glBindTexture(GL_TEXTURE_2D, gBuffer.colorAttachment);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // Fullscreen quad
});

// Post-process pass - no commands needed
bloomPass->SetRenderFunction([this]() {
    // Direct OpenGL - no sorting/batching benefits
    glUseProgram(bloomShader->ID);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
});
```

### Benefits Lost

The core issue is that **different passes have different command system needs**:

#### Passes That Benefit from Commands
- **Geometry Pass**: Many objects, materials, meshes → sorting provides massive benefit
- **Shadow Pass**: Many objects from light perspective → sorting still beneficial
- **Forward Transparent Pass**: Multiple transparent objects → sorting by depth needed

#### Passes That Don't Benefit from Commands
- **Deferred Lighting Pass**: 1 fullscreen quad → no sorting benefit
- **Post-Processing Passes**: 1 quad each → command overhead without benefit  
- **UI Pass**: Different rendering paradigm → direct rendering more appropriate

#### Example: Command System Waste in Simple Passes
```cpp
// Lighting Pass - generates 1 command that doesn't need sorting
void LightingPass::Execute() {
    Begin();
    Renderer::Get().BeginPassCommands();
    
    // Generates 1 command - no sorting benefit
    RenderCommands::DrawElementsData cmd{fullscreenQuadVAO, 6};
    Renderer::Get().Submit(cmd, sortKey); // Overhead without benefit
    
    Renderer::Get().ExecutePassCommands(); // Sorting 1 command is wasteful
    End();
}
```

#### 3. Command Overhead Without Benefits
```cpp
// Instead of:
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

// We do:
RenderCommands::DrawElementsData cmd{vao, 6};
Renderer::Get().Submit(cmd, sortKey);  // Unnecessary overhead
// ...later...
m_CommandBuffer.Sort();                // Sorting 1 command
m_CommandBuffer.Execute();             // std::visit dispatch for 1 command
```

## Architecture Mismatch

The current implementation is a **hybrid approach**:

1. **Uses multi-pass structure** (`RenderPipeline` + `RenderPass`)
2. **But implements single-pass logic** (coordinators doing ECS queries every frame)

### What Should Happen

For true multi-pass rendering:

```cpp
void SceneRenderer::InitializePipeline() {
    // PASS 1: Depth prepass (optional)
    auto depthPass = std::make_shared<RenderPass>("DepthPass", depthSpecs);
    depthPass->SetRenderFunction([this]() {
        // Render all geometry depth-only - no frustum culling needed
        RenderDepthOnly(); 
    });
    
    // PASS 2: Shadow mapping
    auto shadowPass = std::make_shared<RenderPass>("ShadowPass", shadowSpecs);
    shadowPass->SetRenderFunction([this]() {
        // Render from light perspective - different frustum
        RenderFromLight();
    });
    
    // PASS 3: G-Buffer generation
    auto geometryPass = std::make_shared<RenderPass>("GeometryPass", gBufferSpecs);
    geometryPass->SetRenderFunction([this]() {
        // NOW we do frustum culling and mesh rendering
        m_FrustumCuller->CullAgainstCamera(m_Scene.get(), *m_Camera);
        m_MeshRenderer->Render(m_Scene.get(), *m_Camera);
    });
    
    // PASS 4: Deferred lighting
    auto lightingPass = std::make_shared<RenderPass>("LightingPass", screenSpecs);
    lightingPass->SetRenderFunction([this]() {
        // Use G-Buffer - no ECS queries needed
        auto gBuffer = m_Pipeline->GetPass("GeometryPass")->GetFramebuffer();
        ProcessDeferredLighting(gBuffer);
    });
}
```

## Three Strategic Options

### Option 1: Quick Fix - Hybrid Approach (Recommended)

Keep the current architecture but add a **direct rendering path** alongside the command system.

#### Implementation Steps:

1. **Add Direct Rendering Utilities**

Create `lib/Graphics/include/Utility/DirectRenderer.h`:

```cpp
#pragma once
#include <Resources/Shader.h>
#include <Buffer/FrameBuffer.h>
#include <memory>

class DirectRenderer {
public:
    static void Initialize();
    static void Shutdown();
    
    // Direct rendering utilities
    static void RenderFullscreenQuad(std::shared_ptr<Shader> shader);
    static void BindTextures(const std::vector<unsigned int>& textureIDs, 
                           const std::vector<std::string>& uniformNames,
                           std::shared_ptr<Shader> shader);
    
private:
    static unsigned int s_FullscreenQuadVAO;
    static unsigned int s_FullscreenQuadVBO;
};
```

2. **Refactor SceneRenderer for Hybrid Workflow**

```cpp
void SceneRenderer::InitializePipeline() {
    // PASS 1: Geometry (uses command system)
    auto geometryPass = std::make_shared<RenderPass>("GeometryPass", gBufferSpecs);
    geometryPass->SetRenderFunction([this]() {
        // Use command system - benefits from sorting/batching
        m_FrustumCuller->CullAgainstCamera(m_Scene.get(), *m_Camera);
        m_MeshRenderer->Render(m_Scene.get(), *m_Camera);
    });
    
    // PASS 2: Lighting (direct rendering)
    auto lightingPass = std::make_shared<RenderPass>("LightingPass", screenSpecs);
    lightingPass->SetRenderFunction([this]() {
        // Direct rendering - no command system overhead
        RenderDeferredLighting();
    });
    
    m_Pipeline->AddPass(geometryPass);
    m_Pipeline->AddPass(lightingPass);
}

void SceneRenderer::RenderDeferredLighting() {
    auto gBuffer = m_Pipeline->GetPass("GeometryPass")->GetFramebuffer();
    
    m_LightingShader->use();
    
    // Bind G-Buffer textures directly
    std::vector<unsigned int> textures = {
        gBuffer->GetColorAttachmentRendererID(0), // Albedo
        gBuffer->GetColorAttachmentRendererID(1), // Normals
        gBuffer->GetColorAttachmentRendererID(3)  // Depth
    };
    
    DirectRenderer::BindTextures(textures, 
        {"u_GBufferAlbedo", "u_GBufferNormals", "u_GBufferDepth"}, 
        m_LightingShader);
    
    DirectRenderer::RenderFullscreenQuad(m_LightingShader);
}
```

3. **Update Renderer to Support Both Paths**

```cpp
// In Renderer.h
class Renderer {
public:
    // Command-based rendering (existing)
    void BeginFrame();
    void EndFrame();
    void Submit(const VariantRenderCommand& command, const CommandSortKey& sortKey);
    
    // Direct rendering support (new)
    void BeginDirectRendering(); // Skips command buffer
    void EndDirectRendering();   // Skips sorting/execution
};
```

#### Benefits:
- **Preserves existing work** - command system architecture is excellent
- **Immediate benefits** - geometry pass still gets optimal batching
- **Incremental improvement** - add direct rendering without breaking anything
- **Future-proof** - can evolve to Option 2 later

### Option 2: Per-Pass Rendering Modes (Refined Hybrid)

Implement **per-pass rendering modes** where each pass explicitly chooses command-based or direct rendering.

#### Enhanced Architecture:
```cpp
class RenderPass {
public:
    enum class RenderMode {
        CommandBased,  // For passes with many draw calls
        DirectRender   // For passes with few draw calls
    };
    
    void SetRenderMode(RenderMode mode) { m_RenderMode = mode; }
    
    void Execute() override {
        Begin();
        
        if (m_RenderMode == RenderMode::CommandBased) {
            // Use command system - sorting benefits
            Renderer::Get().BeginPassCommands();
            if (m_RenderFunction) m_RenderFunction();
            Renderer::Get().ExecutePassCommands();
        } else {
            // Direct rendering - no command overhead
            if (m_RenderFunction) m_RenderFunction();
        }
        
        End();
    }
    
private:
    RenderMode m_RenderMode = RenderMode::DirectRender;
};
```

#### Usage Example:
```cpp
void SceneRenderer::InitializePipeline() {
    // Geometry pass - MANY draw calls, benefits from sorting
    auto geometryPass = std::make_shared<RenderPass>("GeometryPass", gBufferSpecs);
    geometryPass->SetRenderMode(RenderPass::RenderMode::CommandBased); // Use commands
    geometryPass->SetRenderFunction([this]() {
        m_MeshRenderer->Render(scene, camera); // Submits many commands
    });
    
    // Lighting pass - ONE draw call, no sorting benefit
    auto lightingPass = std::make_shared<RenderPass>("LightingPass", screenSpecs);
    lightingPass->SetRenderMode(RenderPass::RenderMode::DirectRender); // Skip commands
    lightingPass->SetRenderFunction([this]() {
        // Direct OpenGL calls - no command system overhead
        RenderDeferredLighting(); 
    });
    
    m_Pipeline->AddPass(geometryPass);
    m_Pipeline->AddPass(lightingPass);
}
```

#### Benefits:
- **Optimal performance** for each pass type
- **Clean architecture** - explicit about rendering approach
- **Maximum flexibility** - each pass chooses optimal method

#### Drawbacks:
- **Major refactor** required
- **Risk of breaking existing functionality**
- **Time-intensive** implementation

### Option 3: Simplify - Remove Command System (Nuclear)

If most passes don't benefit from commands, **remove the command system entirely** and go back to direct rendering everywhere.

#### Benefits:
- **Simplest implementation**
- **Minimal overhead** everywhere
- **Easy to understand** and maintain

#### Drawbacks:
- **Loses batching benefits** for geometry-heavy passes
- **Wastes excellent command system design**
- **Potential performance regression** for complex scenes

## Recommended Approach

**Start with Option 2 (Per-Pass Rendering Modes)** because it provides the cleanest long-term solution:

### Why Option 2 is Now Preferred

1. **Clear architectural boundaries** - passes explicitly choose their rendering approach
2. **Optimal performance** - each pass uses the most efficient method  
3. **Maintains command system benefits** - geometry-heavy passes still get sorting/batching
4. **Eliminates waste** - simple passes skip unnecessary command overhead
5. **Future-proof** - easy to add new rendering modes as needed

### Clear Performance Benefits per Pass Type

```cpp
// Geometry Pass - Command-based (BENEFITS from sorting)
auto geometryPass = std::make_shared<RenderPass>("GeometryPass", gBufferSpecs);
geometryPass->SetRenderMode(RenderPass::RenderMode::CommandBased);
geometryPass->SetRenderFunction([this]() {
    // Generates 100s-1000s of commands that benefit from sorting
    m_MeshRenderer->Render(scene, camera); // Many draw calls, materials, meshes
});

// Lighting Pass - Direct rendering (AVOIDS command overhead)  
auto lightingPass = std::make_shared<RenderPass>("LightingPass", screenSpecs);
lightingPass->SetRenderMode(RenderPass::RenderMode::DirectRender);
lightingPass->SetRenderFunction([this]() {
    // Direct OpenGL - no command system overhead for 1 draw call
    RenderDeferredLighting(); 
});
```

### Architectural Clarity

```cpp
// Pass execution order: Fixed by pipeline (Level 1)
shadowPass → geometryPass → lightingPass → postProcessPass

// Within geometry pass: Command sorting optimizes draw calls (Level 2)
BindShader(A) → BindShader(A) → BindShader(B) → BindShader(B)
             ↓ (sorted by material)
BindShader(A) → BindShader(A) → BindShader(B) → BindShader(B)

// Within lighting pass: Direct rendering, no commands needed (Level 1 only)
glUseProgram(lightingShader) → glDrawElements(fullscreenQuad)
```

This approach provides **maximum benefit** with **clear architectural boundaries** between pass execution and command optimization.

## Summary

The architectural boundaries clarification makes the decision **much clearer**:

### Key Insights

1. **Two-level architecture** - Pass execution (sequential) + Command optimization (within passes)
2. **Per-pass efficiency needs** - Some passes benefit from commands, others don't
3. **Clear performance gains** - Option 2 provides optimal performance for each pass type

### Final Recommendation: Option 2 (Per-Pass Rendering Modes)

```cpp
// Geometry-heavy passes: Use command system for batching benefits
geometryPass->SetRenderMode(RenderPass::RenderMode::CommandBased);

// Simple passes: Skip command overhead entirely  
lightingPass->SetRenderMode(RenderPass::RenderMode::DirectRender);
```

This provides:
- **Maximum performance** - each pass uses optimal rendering approach
- **Clear architecture** - explicit about command system usage
- **Best of both worlds** - command benefits where needed, efficiency everywhere else
- **Future flexibility** - easy to add new rendering modes (compute, bindless, etc.)

The clarified architectural boundaries make Option 2 the clear winner, providing optimal performance with clean, explicit design.