# **Point Shadows Implementation Plan (Geometry Shader Approach)**

This document describes how to extend the graphics framework to support omnidirectional point light shadows using cubemap depth textures and **geometry shaders** for efficient single-pass rendering, while maintaining the command buffer architecture.

## **Implementation Choice: Geometry Shader (Single-Pass)**

We're using the **geometry shader technique** instead of the traditional 6-pass approach:

**Traditional Approach:**
- Render scene 6 times (once per cubemap face)
- Attach each face individually to FBO
- 6× draw calls per shadow-casting light

**Geometry Shader Approach (Chosen):**
- Render scene **once** per shadow-casting light
- Geometry shader emits geometry to all 6 faces using `gl_Layer`
- **~6× fewer draw calls** - significantly better performance
- Requires OpenGL 3.3+ (geometry shader support)

---

## **Analysis: Current Shadow System**

**Current Implementation (Directional Shadows):**
- `ShadowMappingPass` renders to **single 2048x2048 depth texture**
- Uses **orthographic projection** from light's perspective
- Stores in `FrameData::shadowMaps[0]` and `shadowMatrices[0]`
- Fragment shader samples **sampler2D** for shadow comparison

**Point Shadow Requirements:**
- Need **6 depth textures** (cubemap faces: +X, -X, +Y, -Y, +Z, -Z)
- Or use **cubemap depth texture** (GL_TEXTURE_CUBE_MAP)
- Need **6 view matrices** (one per face) + **perspective projection**
- Fragment shader samples **samplerCube** for shadow comparison

---

## **Design: Minimal Framework Changes**

### **1. Extend FrameData for Cubemap Shadows**

**File:** `lib/Graphics/include/Utility/FrameData.h`

```cpp
struct FrameData {
    // Existing directional shadow data
    std::vector<std::shared_ptr<FrameBuffer>> shadowMaps;
    std::vector<glm::mat4> shadowMatrices;

    // NEW: Point light shadow data
    struct PointShadowData {
        std::shared_ptr<FrameBuffer> cubemapFBO;  // Depth cubemap framebuffer
        std::array<glm::mat4, 6> viewMatrices;    // 6 face view matrices
        glm::mat4 projectionMatrix;               // Shared perspective projection
        glm::vec3 lightPosition;                  // Light world position
        float farPlane;                           // Shadow range
    };

    std::vector<PointShadowData> pointShadows;  // One per point light with shadows

    // ... rest of existing FrameData
};
```

---

### **2. Create Cubemap Framebuffer Support**

**File:** `lib/Graphics/include/Buffer/FrameBuffer.h`

Add new attachment type for cubemap depth:

```cpp
enum class FBOTextureFormat {
    // Existing formats...
    DEPTH24STENCIL8,

    // NEW: Cubemap depth format
    DEPTH_CUBEMAP  // GL_TEXTURE_CUBE_MAP with GL_DEPTH_COMPONENT
};

class FrameBuffer {
public:
    // Existing methods...

    // NEW: Attach specific cubemap face for rendering
    void AttachCubemapFace(uint32_t face, uint32_t mipLevel = 0);

    // NEW: Get cubemap texture ID for shader binding
    uint32_t GetDepthCubemapID() const;
};
```

**File:** `lib/Graphics/src/Buffer/FrameBuffer.cpp`

```cpp
void FrameBuffer::AttachCubemapFace(uint32_t face, uint32_t mipLevel) {
    assert(face >= GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
           face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z &&
           "Invalid cubemap face");

    Bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                          face, m_DepthAttachment, mipLevel);
    Unbind();
}

uint32_t FrameBuffer::GetDepthCubemapID() const {
    return m_DepthAttachment;  // Returns cubemap texture ID
}
```

**Note:** You'll also need to update the `FrameBuffer` constructor to handle `DEPTH_CUBEMAP` format by creating a cubemap texture instead of a 2D texture.

---

### **3. Add BindDepthCubemap Command**

**File:** `lib/Graphics/include/Core/RenderCommandBuffer.h`

```cpp
namespace RenderCommands {
    // ... existing commands ...

    // NEW: Bind depth cubemap for shadow sampling
    struct BindDepthCubemapData {
        unsigned int cubemapID;
        uint32_t textureUnit;           // Which texture slot (e.g., 9 for point shadows)
        std::shared_ptr<Shader> shader;
        std::string uniformName;        // e.g., "u_PointShadowMaps[0]"
    };
}

// Update variant
using VariantRenderCommand = std::variant<
    // ... existing commands ...
    RenderCommands::BindDepthCubemapData
>;
```

**File:** `lib/Graphics/src/Core/RenderCommandBuffer.cpp`

Add the command declaration in the header's private section:

```cpp
private:
    // ... existing ExecuteCommand declarations ...
    void ExecuteCommand(const RenderCommands::BindDepthCubemapData& cmd);
```

Add the implementation:

```cpp
void RenderCommandBuffer::ExecuteCommand(const RenderCommands::BindDepthCubemapData& cmd) {
    assert(cmd.shader && "Shader cannot be null");
    assert(cmd.shader->ID != 0 && "Shader must be compiled");
    assert(cmd.cubemapID != 0 && "Cubemap ID must be valid");
    assert(cmd.textureUnit < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
    assert(!cmd.uniformName.empty() && "Uniform name cannot be empty");

    // Ensure shader is active
    cmd.shader->use();

    // Bind cubemap to texture unit
    glActiveTexture(GL_TEXTURE0 + cmd.textureUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cmd.cubemapID);

    // Set uniform sampler
    cmd.shader->setInt(cmd.uniformName, static_cast<int>(cmd.textureUnit));
}
```

---

### **4. Add SetPointShadowUniforms Command**

**File:** `lib/Graphics/include/Core/RenderCommandBuffer.h`

```cpp
namespace RenderCommands {
    // ... existing commands ...

    // NEW: Set point shadow uniforms (light position, far plane)
    struct SetPointShadowUniformsData {
        std::shared_ptr<Shader> shader;
        glm::vec3 lightPosition;
        float farPlane;
        std::string lightPosUniform;  // e.g., "u_PointShadowLightPositions[0]"
        std::string farPlaneUniform;  // e.g., "u_PointShadowFarPlanes[0]"
    };
}

// Update variant
using VariantRenderCommand = std::variant<
    // ... existing commands ...
    RenderCommands::BindCubemapData,  // Reuse existing command for depth cubemaps
    RenderCommands::SetPointShadowUniformsData
>;
```

**File:** `lib/Graphics/src/Core/RenderCommandBuffer.cpp`

Add the command declaration in the header's private section:

```cpp
private:
    // ... existing ExecuteCommand declarations ...
    void ExecuteCommand(const RenderCommands::SetPointShadowUniformsData& cmd);
```

Add the implementation:

```cpp
void RenderCommandBuffer::ExecuteCommand(const RenderCommands::SetPointShadowUniformsData& cmd) {
    assert(cmd.shader && "Shader cannot be null");
    assert(cmd.shader->ID != 0 && "Shader must be compiled");
    assert(cmd.farPlane > 0.0f && "Far plane must be positive");
    assert(!cmd.lightPosUniform.empty() && "Light position uniform name cannot be empty");
    assert(!cmd.farPlaneUniform.empty() && "Far plane uniform name cannot be empty");

    cmd.shader->use();
    cmd.shader->setVec3(cmd.lightPosUniform, cmd.lightPosition);
    cmd.shader->setFloat(cmd.farPlaneUniform, cmd.farPlane);
}
```

---

### **5. Create PointShadowMappingPass (Geometry Shader Single-Pass)**

**File:** `lib/Graphics/include/Pipeline/PointShadowMappingPass.h`

```cpp
#pragma once

#include "RenderPass.h"
#include "../Utility/RenderData.h"
#include "../Utility/FrameData.h"
#include <memory>
#include <array>
#include <glm/glm.hpp>

class Shader;

/**
 * Point Shadow Mapping Pass - Renders cubemap depth using geometry shader (single-pass)
 *
 * For each point light that casts shadows:
 * 1. Creates depth cubemap framebuffer (1024x1024 per face)
 * 2. Renders scene ONCE using geometry shader that emits to all 6 faces
 * 3. Stores cubemap + matrices in FrameData::pointShadows[]
 *
 * Uses geometry shader technique with gl_Layer for efficient rendering.
 * Fragment shader samples samplerCube for omnidirectional shadows.
 */
class PointShadowMappingPass : public RenderPass {
public:
    PointShadowMappingPass();
    PointShadowMappingPass(std::shared_ptr<Shader> depthShader);
    ~PointShadowMappingPass() = default;

    void Execute(RenderContext& context) override;

    void SetPointShadowDepthShader(std::shared_ptr<Shader> shader) {
        m_PointShadowDepthShader = shader;
    }

    // Configuration
    void SetShadowCubemapSize(uint32_t size) { m_ShadowCubemapSize = size; }
    void SetShadowFarPlane(float farPlane) { m_ShadowFarPlane = farPlane; }
    void SetMaxShadowCastingLights(uint32_t maxLights) {
        m_MaxShadowCastingLights = maxLights;
    }

private:
    // Helper: Calculate 6 view matrices for cubemap faces
    std::array<glm::mat4, 6> CalculateCubemapViewMatrices(const glm::vec3& lightPos);

    // Helper: Calculate perspective projection for point shadows
    glm::mat4 CalculatePointShadowProjection();

    // Helper: Get or create cubemap FBO for a point light
    std::shared_ptr<FrameBuffer> GetOrCreateCubemapFBO(size_t lightIndex);

    // Helper: Render scene to entire cubemap (single pass with geometry shader)
    void RenderToCubemap(RenderContext& context,
                        const std::array<glm::mat4, 6>& viewMatrices,
                        const glm::mat4& projectionMatrix,
                        const glm::vec3& lightPos,
                        size_t lightIndex);

    std::shared_ptr<Shader> m_PointShadowDepthShader;

    // Cubemap FBOs (persistent across frames, one per shadow-casting point light)
    std::vector<std::shared_ptr<FrameBuffer>> m_CubemapFBOs;

    // Configuration
    uint32_t m_ShadowCubemapSize = 1024;
    float m_ShadowFarPlane = 25.0f;
    uint32_t m_MaxShadowCastingLights = 4;  // Max number of point lights with shadows
};
```

**File:** `lib/Graphics/src/Pipeline/PointShadowMappingPass.cpp`

```cpp
#include "../../include/Pipeline/PointShadowMappingPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Utility/Light.h"
#include "../../include/Resources/Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

PointShadowMappingPass::PointShadowMappingPass()
    : RenderPass("PointShadowPass"),  // No default FBO - we create cubemaps dynamically
      m_PointShadowDepthShader(nullptr)
{
}

PointShadowMappingPass::PointShadowMappingPass(std::shared_ptr<Shader> depthShader)
    : RenderPass("PointShadowPass"),
      m_PointShadowDepthShader(depthShader)
{
}

void PointShadowMappingPass::Execute(RenderContext& context) {
    if (!m_PointShadowDepthShader) {
        spdlog::warn("PointShadowMappingPass: No depth shader set, skipping");
        return;
    }

    // Clear previous frame's point shadow data
    context.frameData.pointShadows.clear();

    // Find point lights that should cast shadows
    std::vector<const SubmittedLightData*> shadowCastingLights;
    for (const auto& light : context.lights) {
        if (light.enabled && light.type == Light::Type::Point && light.castShadows) {
            shadowCastingLights.push_back(&light);
            if (shadowCastingLights.size() >= m_MaxShadowCastingLights) {
                break;  // Limit number of shadow-casting point lights
            }
        }
    }

    if (shadowCastingLights.empty()) {
        return;  // No point lights casting shadows
    }

    Begin();
    SetupCommandBuffer(context);

    // Render shadows for each point light (SINGLE PASS with geometry shader)
    for (size_t lightIdx = 0; lightIdx < shadowCastingLights.size(); ++lightIdx) {
        const auto* light = shadowCastingLights[lightIdx];

        // Get or create cubemap FBO for this light
        auto cubemapFBO = GetOrCreateCubemapFBO(lightIdx);

        // Calculate view matrices for 6 cubemap faces
        auto viewMatrices = CalculateCubemapViewMatrices(light->position);
        auto projection = CalculatePointShadowProjection();

        // Render to entire cubemap in ONE pass (geometry shader handles all 6 faces)
        RenderToCubemap(context, viewMatrices, projection, light->position, lightIdx);

        // Store shadow data in FrameData
        FrameData::PointShadowData shadowData;
        shadowData.cubemapFBO = cubemapFBO;
        shadowData.viewMatrices = viewMatrices;
        shadowData.projectionMatrix = projection;
        shadowData.lightPosition = light->position;
        shadowData.farPlane = m_ShadowFarPlane;

        context.frameData.pointShadows.push_back(shadowData);
    }

    End();
}

std::array<glm::mat4, 6> PointShadowMappingPass::CalculateCubemapViewMatrices(
    const glm::vec3& lightPos)
{
    // Standard cubemap view directions and up vectors
    return {
        glm::lookAt(lightPos, lightPos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +X
        glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // -X
        glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)), // +Y
        glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)), // -Y
        glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +Z
        glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))  // -Z
    };
}

glm::mat4 PointShadowMappingPass::CalculatePointShadowProjection() {
    // 90-degree FOV perspective for cubemap faces
    float aspect = 1.0f;  // Cubemap faces are square
    float nearPlane = 0.1f;
    return glm::perspective(glm::radians(90.0f), aspect, nearPlane, m_ShadowFarPlane);
}

std::shared_ptr<FrameBuffer> PointShadowMappingPass::GetOrCreateCubemapFBO(size_t lightIndex) {
    // Resize vector if needed
    if (lightIndex >= m_CubemapFBOs.size()) {
        m_CubemapFBOs.resize(lightIndex + 1);
    }

    // Create FBO if it doesn't exist
    if (!m_CubemapFBOs[lightIndex]) {
        FBOSpecs spec;
        spec.Width = m_ShadowCubemapSize;
        spec.Height = m_ShadowCubemapSize;
        spec.Attachments.Attachments.push_back({FBOTextureFormat::DEPTH_CUBEMAP});

        m_CubemapFBOs[lightIndex] = std::make_shared<FrameBuffer>(spec);
    }

    return m_CubemapFBOs[lightIndex];
}

void PointShadowMappingPass::RenderToCubemap(
    RenderContext& context,
    const std::array<glm::mat4, 6>& viewMatrices,
    const glm::mat4& projectionMatrix,
    const glm::vec3& lightPos,
    size_t lightIndex)
{
    // Attach ENTIRE cubemap to framebuffer (not a specific face)
    // Geometry shader will use gl_Layer to select the face
    auto fbo = m_CubemapFBOs[lightIndex];
    fbo->Bind();

    // Attach all 6 faces using glFramebufferTexture (not glFramebufferTexture2D)
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                        fbo->GetDepthCubemapID(), 0);

    // Set viewport for cubemap
    glViewport(0, 0, m_ShadowCubemapSize, m_ShadowCubemapSize);

    // Clear depth buffer (affects all faces)
    RenderCommands::ClearData clearCmd{
        0.0f, 0.0f, 0.0f, 1.0f,
        false,  // Don't clear color
        true    // Clear depth
    };
    Submit(clearCmd);

    // Bind depth shader (with geometry shader)
    Submit(RenderCommands::BindShaderData{m_PointShadowDepthShader});

    // Pass all 6 shadow matrices to geometry shader
    m_PointShadowDepthShader->use();
    for (int i = 0; i < 6; ++i) {
        std::string matrixName = "u_ShadowMatrices[" + std::to_string(i) + "]";
        m_PointShadowDepthShader->setMat4(matrixName, projectionMatrix * viewMatrices[i]);
    }

    // Pass light position and far plane to fragment shader
    m_PointShadowDepthShader->setVec3("u_LightPos", lightPos);
    m_PointShadowDepthShader->setFloat("u_FarPlane", m_ShadowFarPlane);

    // Render all visible objects ONCE (geometry shader emits to all 6 faces)
    for (const auto& renderable : context.renderables) {
        if (!renderable.visible || !renderable.mesh) continue;

        // Set model matrix uniform
        m_PointShadowDepthShader->setMat4("u_Model", renderable.transform);

        // Draw (geometry shader will emit 6 triangles per input triangle, one per face)
        Submit(RenderCommands::DrawElementsData{
            renderable.mesh->GetVertexArray()->GetVAOHandle(),
            renderable.mesh->GetIndexCount(),
            GL_TRIANGLES
        });
    }

    ExecuteCommands();
    fbo->Unbind();
}
```

---

### **6. Update SubmittedLightData**

**File:** `lib/Graphics/include/Utility/RenderData.h`

```cpp
struct SubmittedLightData {
    Light::Type type = Light::Type::Point;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float range = 10.0f;
    float innerCone = 12.5f;
    float outerCone = 17.5f;
    bool enabled = true;

    // NEW: Shadow casting flag
    bool castShadows = false;  // Default: don't cast shadows
};
```

---

### **7. Create Point Shadow Shaders (with Geometry Shader)**

**File:** `test/examples/lib/Graphics/assets/shaders/point_shadow_depth.vert`

```glsl
#version 330 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_Model;

out vec3 v_FragPos;  // World-space position (pass to geometry shader)

void main() {
    // Transform to world space
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_FragPos = worldPos.xyz;

    // No view/projection here - geometry shader handles that for each face
    gl_Position = worldPos;
}
```

**File:** `test/examples/lib/Graphics/assets/shaders/point_shadow_depth.geom`

```glsl
#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;  // 6 faces × 3 vertices

uniform mat4 u_ShadowMatrices[6];  // 6 shadow matrices (projection * view for each face)

in vec3 v_FragPos[];  // World-space position from vertex shader
out vec4 g_FragPos;   // Output to fragment shader

void main() {
    // Emit one triangle to each of the 6 cubemap faces
    for(int face = 0; face < 6; ++face) {
        gl_Layer = face;  // Built-in variable: selects cubemap face

        for(int i = 0; i < 3; ++i) {
            // Transform vertex to light space for this face
            g_FragPos = gl_in[i].gl_Position;  // World position
            gl_Position = u_ShadowMatrices[face] * g_FragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}
```

**File:** `test/examples/lib/Graphics/assets/shaders/point_shadow_depth.frag`

```glsl
#version 330 core

in vec4 g_FragPos;  // World-space position from geometry shader

uniform vec3 u_LightPos;
uniform float u_FarPlane;

void main() {
    // Calculate distance from light to fragment
    float lightDistance = length(g_FragPos.xyz - u_LightPos);

    // Map to [0, 1] range
    lightDistance = lightDistance / u_FarPlane;

    // Write normalized distance as depth
    gl_FragDepth = lightDistance;
}
```

---

### **8. Update Main PBR Fragment Shader**

**File:** `test/examples/lib/Graphics/assets/shaders/main_pbr.frag`

Add these uniforms after the existing shadow map uniform (around line 33):

```glsl
// Point shadow uniforms
uniform samplerCube u_PointShadowMaps[4];  // Support up to 4 point shadow cubemaps
uniform vec3 u_PointShadowLightPositions[4];
uniform float u_PointShadowFarPlanes[4];
uniform int u_NumPointShadows = 0;
uniform bool u_EnablePointShadows = false;
```

Add this function after the directional shadow calculation function (around line 130):

```glsl
// Point shadow calculation function
float PointShadowCalculation(int shadowIndex, vec3 fragPos) {
    // Get vector from fragment to light
    vec3 fragToLight = fragPos - u_PointShadowLightPositions[shadowIndex];

    // Sample cubemap
    float closestDepth = texture(u_PointShadowMaps[shadowIndex], fragToLight).r;
    closestDepth *= u_PointShadowFarPlanes[shadowIndex];  // Convert back to world units

    // Current fragment distance
    float currentDepth = length(fragToLight);

    // Bias to prevent shadow acne
    float bias = 0.05;

    // Simple shadow test (can be enhanced with PCF later)
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    // Optionally fade shadows at the edge of the shadow range
    if (currentDepth > u_PointShadowFarPlanes[shadowIndex]) {
        shadow = 0.0;
    }

    return shadow;
}
```

Update the `calculatePointLight` function signature (around line 186):

```glsl
// Add shadowFactor parameter
vec3 calculatePointLight(PointLight light, vec3 albedo, vec3 normal, vec3 fragPos,
                        vec3 viewDir, float metallic, float roughness, vec3 F0,
                        float shadowFactor)
{
    // ... existing PBR code ...

    // Apply shadow at the end (before return)
    return (kD * albedo / PI + specular) * radiance * NdotL * (1.0 - shadowFactor * 0.8);
}
```

Update the `main()` function to calculate and apply point shadows (around line 314):

```glsl
void main() {
    // ... existing albedo, metallic, roughness, ao, emissive, normal code ...

    // Calculate directional shadow factor
    float dirShadow = 0.0;
    if (u_EnableShadows) {
        dirShadow = ShadowCalculation(fs_in.FragPosLightSpace);
    }

    // NEW: Calculate point shadows for each shadow-casting point light
    float pointShadows[8];  // Max 8 point lights
    for (int i = 0; i < 8; ++i) {
        pointShadows[i] = 0.0;
    }

    if (u_EnablePointShadows) {
        for (int i = 0; i < u_NumPointShadows && i < 4; ++i) {
            pointShadows[i] = PointShadowCalculation(i, fs_in.FragPos);
        }
    }

    // Calculate multi-light PBR lighting
    vec3 N = normalize(normal);
    vec3 V = normalize(u_ViewPos - fs_in.FragPos);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 Lo = vec3(0.0);

    // Point lights WITH shadows
    for (int i = 0; i < u_NumPointLights && i < 8; ++i) {
        float shadowFactor = (i < u_NumPointShadows) ? pointShadows[i] : 0.0;
        Lo += calculatePointLight(u_PointLights[i], albedo, normal, fs_in.FragPos,
                                 V, metallic, roughness, F0, shadowFactor);
    }

    // Directional lights (with existing directional shadows)
    for (int i = 0; i < u_NumDirectionalLights && i < 4; ++i) {
        Lo += calculateDirectionalLight(u_DirectionalLights[i], albedo, normal, V,
                                       metallic, roughness, F0, dirShadow);
    }

    // Spot lights (no shadows for now)
    for (int i = 0; i < u_NumSpotLights && i < 4; ++i) {
        Lo += calculateSpotLight(u_SpotLights[i], albedo, normal, fs_in.FragPos,
                                V, metallic, roughness, F0);
    }

    // ... rest of existing main() code (ambient, emissive, tone mapping, etc.) ...
}
```

---

### **9. Update InstancedRenderer to Bind Point Shadows**

**File:** `lib/Graphics/src/Rendering/InstancedRenderer.cpp`

Update the `RenderInstancedMeshToPass()` function after the directional shadow uniforms section (around line 268):

```cpp
    // Existing directional shadow code...
    } else {
        // No shadow data available - disable directional shadow mapping
        // ... existing code ...
    }

    // NEW: Set point shadow uniforms if available
    if (!frameData.pointShadows.empty() && frameData.pointShadows.size() <= 4) {
        // Enable point shadows
        shader->use();
        shader->setBool("u_EnablePointShadows", true);
        shader->setInt("u_NumPointShadows", static_cast<int>(frameData.pointShadows.size()));

        // Bind each point shadow cubemap
        for (size_t i = 0; i < frameData.pointShadows.size(); ++i) {
            const auto& pointShadow = frameData.pointShadows[i];

            // Bind cubemap depth texture (slots 9, 10, 11, 12 for up to 4 point shadows)
            uint32_t cubemapTexUnit = 9 + static_cast<uint32_t>(i);
            uint32_t cubemapID = pointShadow.cubemapFBO->GetDepthCubemapID();

            RenderCommands::BindCubemapData cubemapCmd{
                cubemapID,
                cubemapTexUnit,
                shader,
                "u_PointShadowMaps[" + std::to_string(i) + "]"
            };
            renderPass.Submit(cubemapCmd);

            // Set light position and far plane
            RenderCommands::SetPointShadowUniformsData pointShadowCmd{
                shader,
                pointShadow.lightPosition,
                pointShadow.farPlane,
                "u_PointShadowLightPositions[" + std::to_string(i) + "]",
                "u_PointShadowFarPlanes[" + std::to_string(i) + "]"
            };
            renderPass.Submit(pointShadowCmd);
        }
    } else {
        // Disable point shadows
        shader->use();
        shader->setBool("u_EnablePointShadows", false);
    }

    // 6. Bind textures (existing code continues...)
```

---

### **10. Update SceneRenderer to Add Point Shadow Pass**

**File:** `lib/Graphics/include/Scene/SceneRenderer.h`

Add this method declaration in the public section:

```cpp
public:
    // ... existing methods ...

    // Point shadow configuration
    void SetPointShadowDepthShader(const std::shared_ptr<Shader>& shader) const;
```

**File:** `lib/Graphics/src/Scene/SceneRenderer.cpp`

Add the include at the top:

```cpp
#include "Pipeline/PointShadowMappingPass.h"
```

Update `InitializeDefaultPipeline()` around line 50:

```cpp
void SceneRenderer::InitializeDefaultPipeline() {
    auto mainPipeline = std::make_unique<RenderPipeline>();

    // 1. Add directional shadow mapping pass
    auto shadowPass = std::make_shared<ShadowMappingPass>();
    mainPipeline->AddPass(shadowPass);

    // NEW: 2. Add point shadow mapping pass
    auto pointShadowPass = std::make_shared<PointShadowMappingPass>();
    mainPipeline->AddPass(pointShadowPass);

    // 3. Add main rendering pass (now includes skybox rendering)
    auto mainPass = std::make_shared<MainRenderingPass>();
    mainPipeline->AddPass(mainPass);

    // ... rest of existing passes (Debug, Picking, Present) ...

    m_Pipeline = std::move(mainPipeline);
}
```

Add the configuration method implementation at the end of the file:

```cpp
void SceneRenderer::SetPointShadowDepthShader(const std::shared_ptr<Shader>& shader) const {
    assert(shader && "Point shadow depth shader cannot be null");
    assert(shader->ID != 0 && "Point shadow depth shader must be compiled and linked");
    assert(m_Pipeline && "Pipeline must be initialized before setting point shadow shader");

    if (m_Pipeline) {
        auto pointShadowPass = std::dynamic_pointer_cast<PointShadowMappingPass>(
            m_Pipeline->GetPass("PointShadowPass"));
        if (pointShadowPass) {
            pointShadowPass->SetPointShadowDepthShader(shader);
        }
    }
}
```

---

### **11. Add Geometry Shader Support to ResourceManager**

**File:** `lib/Graphics/include/Resources/ResourceManager.h`

Add overload for geometry shader loading:

```cpp
public:
    // Shader management
    std::shared_ptr<Shader> LoadShader(const std::string& name,
                                      const std::string& vertexPath,
                                      const std::string& fragmentPath);

    // NEW: Overload for geometry shader support
    std::shared_ptr<Shader> LoadShader(const std::string& name,
                                      const std::string& vertexPath,
                                      const std::string& geometryPath,
                                      const std::string& fragmentPath);
```

**File:** `lib/Graphics/src/Resources/ResourceManager.cpp`

Add implementation:

```cpp
std::shared_ptr<Shader> ResourceManager::LoadShader(
    const std::string& name,
    const std::string& vertexPath,
    const std::string& geometryPath,
    const std::string& fragmentPath)
{
    // Check if shader already exists
    if (m_Shaders.find(name) != m_Shaders.end()) {
        spdlog::warn("Shader '{}' already loaded. Returning existing shader.", name);
        return m_Shaders[name];
    }

    // Create and compile shader with geometry shader
    auto shader = std::make_shared<Shader>(vertexPath.c_str(),
                                          geometryPath.c_str(),
                                          fragmentPath.c_str());

    if (shader->ID == 0) {
        spdlog::error("Failed to compile shader '{}'", name);
        return nullptr;
    }

    m_Shaders[name] = shader;
    spdlog::info("Shader '{}' loaded successfully (with geometry shader)", name);
    return shader;
}
```

---

### **12. Update Driver to Enable Point Shadows**

**File:** `test/examples/lib/Graphics/main.cpp`

In the `LoadTestResources()` function, add shader loading after the directional shadow shader (around line 240):

```cpp
        // Load shadow mapping depth-only shader
        auto shadowShader = m_ResourceManager->LoadShader("shadow_depth",
            "assets/shaders/shadow_depth.vert",
            "assets/shaders/shadow_depth.frag");

        if (shadowShader) {
            spdlog::info("Shadow mapping shader loaded successfully!");
            m_SceneRenderer->SetShadowDepthShader(shadowShader);
        } else {
            spdlog::warn("Could not load shadow mapping shader");
        }

        // NEW: Load point shadow shader WITH geometry shader
        auto pointShadowShader = m_ResourceManager->LoadShader("point_shadow_depth",
            "assets/shaders/point_shadow_depth.vert",
            "assets/shaders/point_shadow_depth.geom",  // Geometry shader
            "assets/shaders/point_shadow_depth.frag");

        if (pointShadowShader) {
            spdlog::info("Point shadow shader (with geometry shader) loaded successfully!");
            m_SceneRenderer->SetPointShadowDepthShader(pointShadowShader);
        } else {
            spdlog::warn("Could not load point shadow shader");
        }
```

In the `SetupAdvancedScene()` function, enable shadows for specific point lights (around line 427):

```cpp
    // Top-left corner (red light) - WITH SHADOWS
    auto redLight = CreatePointLight(
        glm::vec3(-cornerOffset, lightHeight, -cornerOffset),
        glm::vec3(1.0f, 0.2f, 0.2f),
        4.0f,
        25.0f
    );
    redLight.castShadows = true;  // Enable shadows for this light
    m_SceneLights.push_back(redLight);

    // Top-right corner (green light) - WITH SHADOWS
    auto greenLight = CreatePointLight(
        glm::vec3(cornerOffset, lightHeight, -cornerOffset),
        glm::vec3(0.2f, 1.0f, 0.2f),
        4.0f,
        25.0f
    );
    greenLight.castShadows = true;  // Enable shadows for this light
    m_SceneLights.push_back(greenLight);

    // Bottom-left corner (blue light) - NO SHADOWS
    m_SceneLights.push_back(CreatePointLight(
        glm::vec3(-cornerOffset, lightHeight, cornerOffset),
        glm::vec3(0.2f, 0.2f, 1.0f),
        4.0f,
        25.0f
    ));

    // Bottom-right corner (yellow light) - NO SHADOWS
    m_SceneLights.push_back(CreatePointLight(
        glm::vec3(cornerOffset, lightHeight, cornerOffset),
        glm::vec3(1.0f, 1.0f, 0.2f),
        4.0f,
        25.0f
    ));
```

Add keyboard control for toggling point shadows in `KeyCallback()` (around line 790):

```cpp
            case GLFW_KEY_6:
                s_Instance->ToggleSkybox();
                break;

            // NEW: Key 7 to toggle point shadow pass
            case GLFW_KEY_7:
                s_Instance->ToggleRenderPass("PointShadowPass");
                break;
```

Update the controls info in `Run()` (around line 120):

```cpp
    spdlog::info("  - 5: Toggle object rotation");
    spdlog::info("  - 6: Toggle skybox");
    spdlog::info("  - 7: Toggle point shadows");  // NEW
```

---

## **Summary of Changes**

### **New Files:**
1. `lib/Graphics/include/Pipeline/PointShadowMappingPass.h`
2. `lib/Graphics/src/Pipeline/PointShadowMappingPass.cpp`
3. `test/examples/lib/Graphics/assets/shaders/point_shadow_depth.vert`
4. `test/examples/lib/Graphics/assets/shaders/point_shadow_depth.geom` **(NEW - Geometry Shader)**
5. `test/examples/lib/Graphics/assets/shaders/point_shadow_depth.frag`

### **Modified Files:**
1. `lib/Graphics/include/Utility/FrameData.h` - Add `PointShadowData` struct and `pointShadows` vector
2. `lib/Graphics/include/Utility/RenderData.h` - Add `castShadows` flag to `SubmittedLightData`
3. `lib/Graphics/include/Buffer/FrameBuffer.h` - Add `DEPTH_CUBEMAP` format, `GetDepthCubemapID()`
4. `lib/Graphics/src/Buffer/FrameBuffer.cpp` - Implement cubemap creation in constructor
5. `lib/Graphics/include/Core/RenderCommandBuffer.h` - Add `SetPointShadowUniformsData` command (reuse `BindCubemapData`)
6. `lib/Graphics/src/Core/RenderCommandBuffer.cpp` - Implement `SetPointShadowUniformsData` executor
7. `lib/Graphics/include/Resources/ResourceManager.h` - Add geometry shader overload to `LoadShader()`
8. `lib/Graphics/src/Resources/ResourceManager.cpp` - Implement geometry shader loading
9. `lib/Graphics/include/Scene/SceneRenderer.h` - Add `SetPointShadowDepthShader()` method
10. `lib/Graphics/src/Scene/SceneRenderer.cpp` - Add point shadow pass to pipeline, implement configuration method
11. `lib/Graphics/src/Rendering/InstancedRenderer.cpp` - Bind point shadow cubemaps and uniforms
12. `test/examples/lib/Graphics/assets/shaders/main_pbr.frag` - Add point shadow sampling and application
13. `test/examples/lib/Graphics/main.cpp` - Load point shadow shader (with geometry shader), enable shadows on lights, add keyboard control

### **Architecture Compliance:**
✅ **Command buffer used throughout** - `PointShadowMappingPass` submits commands via `Submit()`
✅ **Pass-based architecture** - New pass integrates cleanly into pipeline execution order
✅ **No raw OpenGL in rendering coordinators** - `InstancedRenderer` uses `BindCubemapData` command
✅ **Reuses existing commands** - `BindCubemapData` works for both skybox and depth cubemaps

⚠️ **Necessary exceptions (unavoidable with current architecture):**
   1. `glFramebufferTexture()` in `RenderToCubemap()` - Attaches entire cubemap for layered rendering (no command abstraction needed)
   2. Shader uniform arrays in `RenderToCubemap()` - Setting `u_ShadowMatrices[6]` array (could add batch uniform command if needed)

### **Performance Considerations (Geometry Shader Approach):**
- Each shadow-casting point light requires **1 render pass** (geometry shader renders all 6 faces simultaneously)
- **~6x fewer draw calls** compared to traditional 6-pass approach
- Cubemap resolution of 1024x1024 per face (adjustable via `SetShadowCubemapSize()`)
- Maximum of 4 shadow-casting point lights by default (adjustable via `SetMaxShadowCastingLights()`)
- Cubemap FBOs are **persistent across frames** to avoid recreation overhead
- Geometry shader overhead is minimal on modern GPUs (OpenGL 3.3+)

**Performance Comparison** (100 objects, 2 shadow-casting point lights):
- Traditional 6-pass: 6 × 100 × 2 = **1200 draw calls**
- Geometry shader: 1 × 100 × 2 = **200 draw calls** (6x improvement)

### **Testing Steps:**
1. Build the project with the new files
2. Run the graphics test driver
3. Press `7` to toggle point shadows on/off
4. Observe omnidirectional shadows from the red and green point lights
5. Move the camera to see shadows cast in all directions from the lights

The design maintains your framework's command buffer architecture while adding the minimal infrastructure needed for omnidirectional point light shadows!
