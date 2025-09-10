# Physically Based Rendering (PBR) Implementation Guide

## Overview

This document explains how Physically Based Rendering (PBR) is implemented in the graphics engine, including the mathematical foundations, shader implementation, and light interaction system.

## Table of Contents
- [PBR Fundamentals](#pbr-fundamentals)
- [Material System](#material-system)
- [Lighting Model](#lighting-model)
- [Shader Implementation](#shader-implementation)
- [Texture System](#texture-system)
- [Multi-Light Support](#multi-light-support)
- [Implementation Details](#implementation-details)

## PBR Fundamentals

### What is PBR?
Physically Based Rendering (PBR) is a rendering approach that aims to simulate light interaction with surfaces based on real-world physics principles. This results in more realistic and consistent lighting across different environments.

### Core Principles
1. **Energy Conservation**: The amount of light reflected never exceeds the amount of incoming light
2. **Metallic/Dielectric Workflow**: Materials are categorized as either metallic or dielectric (non-metallic)
3. **Linear Color Space**: All calculations are performed in linear color space
4. **Fresnel Reflectance**: Surface reflectance varies based on viewing angle

### Material Properties

#### Albedo (Base Color)
- **Purpose**: The base diffuse color of the material
- **Range**: RGB values [0-1] in linear space
- **Usage**: For metals, this represents the specular color; for dielectrics, the diffuse color

#### Metallic
- **Purpose**: Defines whether a material is metallic or dielectric
- **Range**: 0.0 (dielectric) to 1.0 (metallic)
- **Effect**: 
  - Metallic materials have no diffuse reflection
  - Dielectric materials have both diffuse and specular reflection

#### Roughness
- **Purpose**: Controls how rough or smooth the surface appears
- **Range**: 0.0 (mirror-like) to 1.0 (completely rough)
- **Effect**: Determines the size and intensity of specular highlights

#### Ambient Occlusion (AO)
- **Purpose**: Simulates ambient lighting occlusion in crevices
- **Range**: 0.0 (fully occluded) to 1.0 (no occlusion)
- **Effect**: Darkens areas where ambient light would be blocked

## Material System

### Material Class Structure
```cpp
class Material {
private:
    std::shared_ptr<Shader> m_Shader;
    std::string m_Name;
public:
    void SetFloat(const std::string& name, float value);
    void SetVec3(const std::string& name, const glm::vec3& value);
    // ... other uniform setters
};
```

### Current Material Setup
The engine currently uses uniform material properties:
- **Albedo Color**: `glm::vec3(0.8f, 0.7f, 0.6f)` - Warm metallic base
- **Metallic Value**: `0.7f` - Mostly metallic with some dielectric properties
- **Roughness Value**: `0.3f` - Fairly smooth surface with some roughness

## Lighting Model

### Supported Light Types

#### Point Lights
```cpp
struct PointLight {
    glm::vec3 position;    // World space position
    glm::vec3 color;       // RGB color
    float intensity;       // Light intensity multiplier
    float constant;        // Constant attenuation
    float linear;          // Linear attenuation
    float quadratic;       // Quadratic attenuation
};
```

**Attenuation Formula**:
```
attenuation = 1.0 / (constant + linear * distance + quadratic * distance²)
```

#### Directional Lights
```cpp
struct DirectionalLight {
    glm::vec3 direction;   // Light direction (world space)
    glm::vec3 color;       // RGB color
    float intensity;       // Light intensity multiplier
};
```

#### Spot Lights
```cpp
struct SpotLight {
    glm::vec3 position;    // World space position
    glm::vec3 direction;   // Light direction
    glm::vec3 color;       // RGB color
    float intensity;       // Light intensity multiplier
    float cutOff;          // Inner cone angle (cosine)
    float outerCutOff;     // Outer cone angle (cosine)
    float constant;        // Constant attenuation
    float linear;          // Linear attenuation
    float quadratic;       // Quadratic attenuation
};
```

**Spotlight Calculation**:
```glsl
float theta = dot(lightDirection, normalize(-spotLight.direction));
float epsilon = cutOff - outerCutOff;
float intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);
```

## Shader Implementation

### Cook-Torrance BRDF Model

The engine implements the Cook-Torrance BRDF (Bidirectional Reflectance Distribution Function):

```
BRDF = kd * (albedo / π) + ks * (DFG / 4(ωo · n)(ωi · n))
```

Where:
- **kd**: Diffuse reflection ratio
- **ks**: Specular reflection ratio  
- **D**: Normal Distribution Function (Trowbridge-Reitz GGX)
- **F**: Fresnel Function (Schlick approximation)
- **G**: Geometry Function (Smith's method)

### Key Shader Functions

#### Normal Distribution Function (D)
```glsl
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return a2 / denom;
}
```

#### Fresnel Function (F)
```glsl
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
```

#### Geometry Function (G)
```glsl
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float ggx2 = GeometrySchlickGGX(max(dot(N, V), 0.0), roughness);
    float ggx1 = GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
    return ggx1 * ggx2;
}
```

## Texture System

### Bindless Texture Support
The engine uses OpenGL ARB_bindless_texture extension for efficient texture access:

```cpp
// Texture handles are stored as 64-bit integers
layout(std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};

// Textures are accessed directly in shaders
vec4 albedo = texture(sampler2D(textureHandle), texCoords);
```

### Current Texture Setup
The engine currently loads textures through the model loading system:
- **Diffuse Maps**: Used for albedo information
- **Normal Maps**: For surface detail (if available)
- **Metallic/Roughness Maps**: Combined or separate maps (if available)

## Multi-Light Support

### Light Configuration
The current setup includes:
- **4 Point Lights**: Positioned at grid corners with different colors (Red, Green, Blue, Yellow)
- **1 Directional Light**: Main sun light with warm white color
- **2 Spot Lights**: Dramatic accent lighting with warm and purple colors

### Light Setup in Code
```cpp
void SetupLighting() {
    auto* instancedRenderer = GetSceneRenderer()->GetInstancedRenderer();
    
    // Point lights at corners
    InstancedRenderer::PointLight pointLight1{
        glm::vec3(-12.0f, 8.0f, -12.0f),  // position
        glm::vec3(1.0f, 0.2f, 0.2f),      // color: Red
        15.0f,                             // intensity
        1.0f, 0.09f, 0.032f               // attenuation
    };
    instancedRenderer->AddPointLight(pointLight1);
    // ... more lights
}
```

## Implementation Details

### Instanced Rendering Integration
The PBR system works seamlessly with instanced rendering:
- Material properties can be stored per-instance in SSBO
- Light calculations are performed per-fragment
- Bindless textures allow different textures per instance

### Performance Considerations
1. **Shader Complexity**: PBR shaders are computationally intensive
2. **Light Count**: Each additional light increases fragment shader cost
3. **Texture Access**: Bindless textures reduce state changes
4. **Instancing**: Reduces draw calls for multiple objects

### Forward Rendering Pipeline
The current implementation uses forward rendering:
1. **Geometry Pass**: Vertex transformation and attribute interpolation
2. **Lighting Pass**: PBR calculations for all lights per fragment
3. **Output**: Final lit color directly to framebuffer

### File Locations
- **Shaders**: `test/examples/lib/Graphics/assets/shaders/instanced_bindless.*`
- **InstancedRenderer**: `lib/Graphics/src/Rendering/InstancedRenderer.cpp`
- **Material System**: `lib/Graphics/include/Resources/Material.h`
- **Light Setup**: `test/examples/lib/Graphics/main.cpp` (SetupLighting method)

## Mathematical References

### BRDF Equation
```
Lo(p,ωo) = ∫Ω [kd * (c/π) + ks * (DFG)/(4(ωo·n)(ωi·n))] * Li(p,ωi) * (n·ωi) * dωi
```

Where:
- **Lo**: Outgoing radiance
- **p**: Surface point
- **ωo**: Outgoing light direction (view direction)  
- **ωi**: Incoming light direction
- **n**: Surface normal
- **c**: Albedo color
- **Li**: Incoming radiance

### Energy Conservation
```
kd + ks = 1.0
kd = kd * (1.0 - ks)
```

This ensures that the material never reflects more light than it receives.

---

*This guide provides a comprehensive overview of the PBR implementation in the graphics engine. For specific implementation details, refer to the shader files and rendering code mentioned above.*