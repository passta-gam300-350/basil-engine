# Texture Types and Flags in Bindless Texture Implementation

## Overview

This document explains how texture types and flags work in the bindless texture system. These components provide semantic meaning and metadata for textures, enabling sophisticated material systems and runtime validation.

## Table of Contents

1. [Texture Types](#texture-types)
2. [Texture Flags](#texture-flags)
3. [Data Structure Layout](#data-structure-layout)
4. [Implementation Details](#implementation-details)
5. [Shader Integration](#shader-integration)
6. [Practical Examples](#practical-examples)
7. [Advanced Usage](#advanced-usage)
8. [Debugging and Validation](#debugging-and-validation)

---

## Texture Types

### Purpose

Texture types categorize textures by their **semantic purpose** in the material/lighting system. They tell the shader what each texture represents and how to use it in rendering calculations.

### Type Definitions

```cpp
// Texture Type Mapping in GetTextureTypeIndex()
enum TextureType : uint32_t {
    DIFFUSE   = 0,  // "texture_diffuse"   - Base color/albedo
    NORMAL    = 1,  // "texture_normal"    - Surface normal perturbation
    METALLIC  = 2,  // "texture_metallic"  - Metallic workflow parameter
    ROUGHNESS = 3,  // "texture_roughness" - Surface roughness
    AO        = 4,  // "texture_ao"        - Ambient occlusion
    EMISSIVE  = 5,  // "texture_emissive"  - Self-illumination
    SPECULAR  = 6,  // "texture_specular"  - Specular reflection
    HEIGHT    = 7   // "texture_height"    - Height/displacement mapping
};
```

### Implementation in Code

```cpp
uint32_t BindlessTextureBinding::GetTextureTypeIndex(const std::string& type) {
    if (type == "texture_diffuse")   return 0;  // Base color
    if (type == "texture_normal")    return 1;  // Normal mapping
    if (type == "texture_metallic")  return 2;  // PBR metallic
    if (type == "texture_roughness") return 3;  // PBR roughness
    if (type == "texture_ao")        return 4;  // Ambient occlusion
    if (type == "texture_emissive")  return 5;  // Glow/emission
    if (type == "texture_specular")  return 6;  // Specular highlights
    if (type == "texture_height")    return 7;  // Displacement/parallax
    return 0; // Default to diffuse
}
```

### Detailed Type Descriptions

#### 1. **Diffuse (Albedo) - Type 0**
- **Purpose**: Base color of the material
- **Data**: RGB color information
- **Usage**: Primary color before lighting calculations
- **Example**: Wood grain color, metal tint, fabric pattern

#### 2. **Normal Map - Type 1**
- **Purpose**: Surface detail and bump information
- **Data**: Encoded normal vectors (RGB → XYZ)
- **Usage**: Perturbs surface normals for lighting detail
- **Example**: Brick texture bumps, metal scratches, fabric weave

#### 3. **Metallic Map - Type 2**
- **Purpose**: Defines metallic vs dielectric materials
- **Data**: Grayscale values (0.0 = dielectric, 1.0 = metallic)
- **Usage**: PBR metallic workflow parameter
- **Example**: Pure metal areas vs painted/plastic areas

#### 4. **Roughness Map - Type 3**
- **Purpose**: Surface microsurface roughness
- **Data**: Grayscale values (0.0 = mirror, 1.0 = completely rough)
- **Usage**: Controls specular reflection spread
- **Example**: Polished metal (low) vs sandpaper (high)

#### 5. **Ambient Occlusion - Type 4**
- **Purpose**: Self-shadowing information
- **Data**: Grayscale occlusion values (0.0 = occluded, 1.0 = exposed)
- **Usage**: Darkens areas that receive less ambient light
- **Example**: Crevices in wood grain, gaps between fabric threads

#### 6. **Emissive Map - Type 5**
- **Purpose**: Self-illuminating areas
- **Data**: RGB emission color and intensity
- **Usage**: Adds light contribution independent of external lighting
- **Example**: LED strips, glowing elements, screens

#### 7. **Specular Map - Type 6**
- **Purpose**: Specular reflection control (legacy workflow)
- **Data**: Grayscale or RGB specular values
- **Usage**: Controls specular intensity in non-PBR workflows
- **Example**: Wet surfaces, glossy paint, polished surfaces

#### 8. **Height/Displacement Map - Type 7**
- **Purpose**: Surface height variation
- **Data**: Grayscale height values
- **Usage**: Parallax mapping, tessellation displacement
- **Example**: Brick height variation, cobblestone displacement

---

## Texture Flags

### Purpose

Flags provide **metadata and state information** about textures. They enable validation, optimization hints, and debugging information.

### Current Flag System

```cpp
// Basic implementation in UpdateHandleData()
handleData.flags = 1; // Mark as valid (bit 0)

// Flag definitions
enum TextureFlags : uint32_t {
    TEXTURE_VALID = (1 << 0)  // Bit 0: Texture is valid and ready to use
};
```

### Extended Flag System (Potential Enhancement)

```cpp
enum TextureFlags : uint32_t {
    // Validity and state
    TEXTURE_VALID       = (1 << 0),  // Bit 0: Texture is valid
    TEXTURE_RESIDENT    = (1 << 1),  // Bit 1: Handle is GPU-resident
    TEXTURE_LOADING     = (1 << 2),  // Bit 2: Currently loading
    TEXTURE_ERROR       = (1 << 3),  // Bit 3: Load/creation error
    
    // Format information
    TEXTURE_SRGB        = (1 << 4),  // Bit 4: sRGB color space
    TEXTURE_HDR         = (1 << 5),  // Bit 5: High dynamic range
    TEXTURE_COMPRESSED  = (1 << 6),  // Bit 6: Compressed format
    TEXTURE_ALPHA       = (1 << 7),  // Bit 7: Has alpha channel
    
    // Features and capabilities
    TEXTURE_MIPMAPPED   = (1 << 8),  // Bit 8: Has mipmaps
    TEXTURE_CUBEMAP     = (1 << 9),  // Bit 9: Cubemap texture
    TEXTURE_ARRAY       = (1 << 10), // Bit 10: Texture array
    TEXTURE_MULTISAMPLED = (1 << 11), // Bit 11: Multisampled
    
    // Usage hints
    TEXTURE_ANIMATED    = (1 << 12), // Bit 12: Animated/dynamic
    TEXTURE_STREAMING   = (1 << 13), // Bit 13: LOD streaming
    TEXTURE_TEMPORARY   = (1 << 14), // Bit 14: Temporary/cache
    TEXTURE_DEBUG       = (1 << 15), // Bit 15: Debug visualization
    
    // Quality levels
    TEXTURE_LOD_HIGH    = (1 << 16), // Bit 16: High quality LOD
    TEXTURE_LOD_MEDIUM  = (1 << 17), // Bit 17: Medium quality LOD
    TEXTURE_LOD_LOW     = (1 << 18), // Bit 18: Low quality LOD
    
    // Rendering hints
    TEXTURE_LINEAR      = (1 << 19), // Bit 19: Linear filtering
    TEXTURE_NEAREST     = (1 << 20), // Bit 20: Nearest filtering
    TEXTURE_CLAMP       = (1 << 21), // Bit 21: Clamp addressing
    TEXTURE_REPEAT      = (1 << 22), // Bit 22: Repeat addressing
};
```

### Flag Usage Examples

```cpp
// Setting multiple flags
uint32_t flags = TEXTURE_VALID | TEXTURE_SRGB | TEXTURE_MIPMAPPED | TEXTURE_RESIDENT;

// Checking specific flags
bool isValid = (flags & TEXTURE_VALID) != 0;
bool isHDR = (flags & TEXTURE_HDR) != 0;
bool hasAlpha = (flags & TEXTURE_ALPHA) != 0;

// Conditional operations based on flags
if (flags & TEXTURE_COMPRESSED) {
    // Handle compressed texture format
}
if (flags & TEXTURE_STREAMING) {
    // Update LOD based on distance
}
```

---

## Data Structure Layout

### GPU Memory Organization

```
SSBO Memory Layout:
┌─────────────────────────────────────┐
│ Handles Array (GLuint64)            │
│ [handle0][handle1]...[handleN]       │ ← Texture GPU pointers
├─────────────────────────────────────┤
│ Types Array (uint32_t)              │
│ [type0][type1]...[typeN]            │ ← Semantic meaning (0-7)
├─────────────────────────────────────┤
│ Flags Array (uint32_t)              │
│ [flags0][flags1]...[flagsN]         │ ← Metadata and state
└─────────────────────────────────────┘
```

### Example Data Layout

```
For a PBR material with 4 textures:

Index:  Handle:           Type:  Type Name:        Flags:
[0]     0x7F8A4B2C1D3E    0      texture_diffuse   0x00000101 (VALID|SRGB)
[1]     0x2B1C8F5A7E9D    1      texture_normal    0x00000001 (VALID)
[2]     0x9E3F6C4B2A18    2      texture_metallic  0x00000001 (VALID)
[3]     0x4D7A2E5F8C91    3      texture_roughness 0x00000001 (VALID)
```

### C++ Data Structure

```cpp
struct TextureHandleData {
    GLuint64 handle;    // 8 bytes - bindless texture handle
    uint32_t type;      // 4 bytes - semantic type (0-7)
    uint32_t flags;     // 4 bytes - metadata flags
    // Total: 16 bytes per texture (std430 aligned)
};

// Usage in bindless system
std::vector<TextureHandleData> m_HandleData;
```

---

## Implementation Details

### Type Assignment Process

```cpp
void BindlessTextureBinding::UpdateHandleData(const std::vector<Texture>& textures) {
    m_HandleData.clear();
    m_HandleData.reserve(textures.size());
    
    for (const auto& texture : textures) {
        GLuint64 handle = GetOrCreateHandle(texture.id);
        
        if (handle != 0) {
            TextureHandleData handleData;
            handleData.handle = handle;                           // GPU pointer
            handleData.type = GetTextureTypeIndex(texture.type);  // Semantic type
            handleData.flags = TEXTURE_VALID;                     // Mark as valid
            
            // Add additional flags based on texture properties
            if (texture.path.find("_sRGB") != std::string::npos) {
                handleData.flags |= TEXTURE_SRGB;
            }
            if (texture.path.find("_HDR") != std::string::npos) {
                handleData.flags |= TEXTURE_HDR;
            }
            
            m_HandleData.push_back(handleData);
        }
    }
    
    m_SSBODirty = true; // Need to upload to GPU
}
```

### GPU Upload Process

```cpp
void BindlessTextureBinding::UpdateSSBO() {
    // Separate data into three parallel arrays
    std::vector<GLuint64> handles;
    std::vector<uint32_t> types;
    std::vector<uint32_t> flags;
    
    for (const auto& data : m_HandleData) {
        handles.push_back(data.handle);
        types.push_back(data.type);
        flags.push_back(data.flags);
    }
    
    // Upload each array to its offset in SSBO
    size_t handleOffset = 0;
    size_t typeOffset = handles.size() * sizeof(GLuint64);
    size_t flagOffset = typeOffset + types.size() * sizeof(uint32_t);
    
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, handleOffset, 
                    handles.size() * sizeof(GLuint64), handles.data());
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, typeOffset,
                    types.size() * sizeof(uint32_t), types.data());
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, flagOffset,
                    flags.size() * sizeof(uint32_t), flags.data());
}
```

---

## Shader Integration

### GLSL SSBO Definition

```glsl
#version 450 core
#extension GL_ARB_bindless_texture : require

// SSBO must match C++ layout exactly
layout(std430, binding = 1) readonly buffer TextureHandles {
    uvec2 handles[];        // 64-bit handles stored as 2×32-bit
    uint types[];           // Texture semantic types
    uint flags[];           // Texture metadata flags
} textureData;

// Uniform indices pointing into SSBO arrays
uniform int u_DiffuseIndex;
uniform int u_NormalIndex;
uniform int u_MetallicIndex;
uniform int u_RoughnessIndex;

// Availability flags for optimization
uniform bool u_HasDiffuseMap;
uniform bool u_HasNormalMap;
uniform bool u_HasMetallicMap;
uniform bool u_HasRoughnessMap;
```

### Safe Texture Sampling Function

```glsl
vec4 SampleTexture(int index, vec2 coords) {
    // Validate index bounds
    if (index < 0 || index >= textureData.handles.length()) {
        return vec4(1.0); // White fallback
    }
    
    // Reconstruct 64-bit handle from two 32-bit parts
    uint64_t handle = packUint2x32(textureData.handles[index]);
    
    // Check validity flag and handle
    if ((textureData.flags[index] & 1u) != 0u && handle != 0ul) {
        // Cast handle to sampler and sample
        return texture(sampler2D(handle), coords);
    }
    
    return vec4(1.0); // Fallback for invalid textures
}
```

### Type-Aware Sampling

```glsl
vec4 SampleTextureWithType(int index, vec2 coords) {
    if (index < 0 || index >= textureData.handles.length()) {
        return vec4(1.0);
    }
    
    uint textureType = textureData.types[index];
    uint textureFlags = textureData.flags[index];
    
    // Validate texture
    if ((textureFlags & 1u) == 0u) return vec4(1.0);
    
    uint64_t handle = packUint2x32(textureData.handles[index]);
    if (handle == 0ul) return vec4(1.0);
    
    vec4 texValue = texture(sampler2D(handle), coords);
    
    // Apply type-specific processing
    switch (textureType) {
        case 0: // Diffuse
            // Apply sRGB correction if flagged
            if ((textureFlags & 16u) != 0u) { // TEXTURE_SRGB flag
                texValue.rgb = pow(texValue.rgb, vec3(2.2));
            }
            break;
            
        case 1: // Normal
            // Decode normal map
            texValue.xyz = texValue.xyz * 2.0 - 1.0;
            break;
            
        case 5: // Emissive
            // Scale emissive values for HDR
            if ((textureFlags & 32u) != 0u) { // TEXTURE_HDR flag
                texValue.rgb *= 4.0; // HDR scaling
            }
            break;
    }
    
    return texValue;
}
```

---

## Practical Examples

### Example 1: Complete PBR Material Setup

```cpp
// C++ Material Creation
void CreatePBRMaterial() {
    std::vector<Texture> pbrTextures = {
        {albedoTexId, "texture_diffuse", "metal_albedo.png"},      // Type 0
        {normalTexId, "texture_normal", "metal_normal.png"},       // Type 1
        {metallicTexId, "texture_metallic", "metal_metallic.png"}, // Type 2
        {roughTexId, "texture_roughness", "metal_rough.png"},      // Type 3
        {aoTexId, "texture_ao", "metal_ao.png"},                   // Type 4
        {emissiveTexId, "texture_emissive", "metal_glow.png"}      // Type 5
    };
    
    // Bind to shader
    bindlessSystem->BindTextures(pbrTextures, pbrShader);
    
    // Set texture indices
    pbrShader->setInt("u_DiffuseIndex", 0);
    pbrShader->setInt("u_NormalIndex", 1);
    pbrShader->setInt("u_MetallicIndex", 2);
    pbrShader->setInt("u_RoughnessIndex", 3);
    pbrShader->setInt("u_AOIndex", 4);
    pbrShader->setInt("u_EmissiveIndex", 5);
    
    // Set availability flags
    pbrShader->setBool("u_HasDiffuseMap", true);
    pbrShader->setBool("u_HasNormalMap", true);
    pbrShader->setBool("u_HasMetallicMap", true);
    pbrShader->setBool("u_HasRoughnessMap", true);
    pbrShader->setBool("u_HasAOMap", true);
    pbrShader->setBool("u_HasEmissiveMap", true);
}
```

```glsl
// GLSL PBR Fragment Shader
void main() {
    // Sample all texture types
    vec3 albedo = u_HasDiffuseMap ? 
        SampleTexture(u_DiffuseIndex, TexCoords).rgb : 
        vec3(0.8);
    
    vec3 normal = u_HasNormalMap ? 
        SampleNormal(u_NormalIndex, TexCoords) : 
        normalize(Normal);
    
    float metallic = u_HasMetallicMap ? 
        SampleTexture(u_MetallicIndex, TexCoords).r : 
        0.0;
    
    float roughness = u_HasRoughnessMap ? 
        SampleTexture(u_RoughnessIndex, TexCoords).r : 
        0.5;
    
    float ao = u_HasAOMap ? 
        SampleTexture(u_AOIndex, TexCoords).r : 
        1.0;
    
    vec3 emissive = u_HasEmissiveMap ? 
        SampleTexture(u_EmissiveIndex, TexCoords).rgb : 
        vec3(0.0);
    
    // Calculate PBR lighting
    vec3 color = calculatePBR(albedo, normal, metallic, roughness, ao);
    color += emissive;
    
    // Tone mapping and gamma correction
    color = color / (color + vec3(1.0)); // Reinhard
    color = pow(color, vec3(1.0/2.2));   // Gamma
    
    FragColor = vec4(color, 1.0);
}
```

### Example 2: Multi-Material Dynamic Selection

```cpp
// C++ Multi-Material Setup
void SetupMultipleMaterials() {
    std::vector<Texture> allTextures;
    
    // Material 0: Wood (4 textures)
    allTextures.push_back({woodAlbedo, "texture_diffuse", "wood_albedo.png"});    // Index 0
    allTextures.push_back({woodNormal, "texture_normal", "wood_normal.png"});     // Index 1
    allTextures.push_back({woodRough, "texture_roughness", "wood_rough.png"});    // Index 2
    allTextures.push_back({woodAO, "texture_ao", "wood_ao.png"});                 // Index 3
    
    // Material 1: Metal (4 textures)  
    allTextures.push_back({metalAlbedo, "texture_diffuse", "metal_albedo.png"});  // Index 4
    allTextures.push_back({metalNormal, "texture_normal", "metal_normal.png"});   // Index 5
    allTextures.push_back({metalMetal, "texture_metallic", "metal_metallic.png"}); // Index 6
    allTextures.push_back({metalRough, "texture_roughness", "metal_rough.png"});  // Index 7
    
    // Material 2: Fabric (4 textures)
    allTextures.push_back({fabricAlbedo, "texture_diffuse", "fabric_albedo.png"}); // Index 8
    allTextures.push_back({fabricNormal, "texture_normal", "fabric_normal.png"});  // Index 9
    allTextures.push_back({fabricRough, "texture_roughness", "fabric_rough.png"}); // Index 10
    allTextures.push_back({fabricAO, "texture_ao", "fabric_ao.png"});              // Index 11
    
    bindlessSystem->BindTextures(allTextures, multiMatShader);
}
```

```glsl
// GLSL Multi-Material Shader
uniform int u_MaterialID;     // 0=wood, 1=metal, 2=fabric
uniform int u_TexturesPerMaterial = 4;

void main() {
    // Calculate base index for this material
    int baseIndex = u_MaterialID * u_TexturesPerMaterial;
    
    // Determine texture indices for this material
    int diffuseIndex = baseIndex + 0;
    int normalIndex = baseIndex + 1;
    int materialIndex = baseIndex + 2;   // Could be roughness or metallic
    int aoIndex = baseIndex + 3;
    
    // Sample textures for selected material
    vec3 albedo = SampleTexture(diffuseIndex, TexCoords).rgb;
    vec3 normal = SampleNormal(normalIndex, TexCoords);
    float ao = SampleTexture(aoIndex, TexCoords).r;
    
    // Handle different material properties
    float metallic = 0.0;
    float roughness = 0.5;
    
    // Check texture type at material index
    uint matTexType = textureData.types[materialIndex];
    if (matTexType == 2) {      // METALLIC type
        metallic = SampleTexture(materialIndex, TexCoords).r;
    } else if (matTexType == 3) { // ROUGHNESS type
        roughness = SampleTexture(materialIndex, TexCoords).r;
    }
    
    // Adjust defaults based on material type
    if (u_MaterialID == 0) {        // Wood
        metallic = 0.0;             // Non-metallic
        roughness = max(roughness, 0.6); // Minimum roughness
    } else if (u_MaterialID == 1) { // Metal
        metallic = max(metallic, 0.8);   // Mostly metallic
        roughness = min(roughness, 0.3); // Maximum smoothness
    }
    
    // Calculate final color
    vec3 color = calculatePBR(albedo, normal, metallic, roughness, ao);
    FragColor = vec4(color, 1.0);
}
```

---

## Advanced Usage

### Dynamic LOD Selection Based on Flags

```glsl
vec4 SampleTextureWithLOD(int baseIndex, vec2 coords, float distance) {
    // Check available LOD levels using flags
    int lodIndex = baseIndex;
    
    if (distance > 50.0) {
        // Try to find low LOD version
        for (int i = 0; i < 3; i++) {
            int testIndex = baseIndex + i;
            if (testIndex < textureData.flags.length() && 
                (textureData.flags[testIndex] & (1u << 18)) != 0u) { // TEXTURE_LOD_LOW
                lodIndex = testIndex;
                break;
            }
        }
    } else if (distance > 20.0) {
        // Try to find medium LOD version
        for (int i = 0; i < 3; i++) {
            int testIndex = baseIndex + i;
            if (testIndex < textureData.flags.length() && 
                (textureData.flags[testIndex] & (1u << 17)) != 0u) { // TEXTURE_LOD_MEDIUM
                lodIndex = testIndex;
                break;
            }
        }
    }
    // else use high LOD (baseIndex)
    
    return SampleTexture(lodIndex, coords);
}
```

### Conditional Processing Based on Texture Type

```glsl
vec3 ProcessTextureByType(int index, vec2 coords) {
    if (index < 0 || index >= textureData.types.length()) {
        return vec3(1.0);
    }
    
    uint texType = textureData.types[index];
    uint texFlags = textureData.flags[index];
    vec4 texValue = SampleTexture(index, coords);
    
    switch (texType) {
        case 0: // Diffuse
            if ((texFlags & (1u << 4)) != 0u) { // TEXTURE_SRGB
                return pow(texValue.rgb, vec3(2.2)); // sRGB to linear
            }
            return texValue.rgb;
            
        case 1: // Normal
            vec3 normal = texValue.xyz * 2.0 - 1.0;
            return normalize(normal);
            
        case 2: // Metallic
        case 3: // Roughness
        case 4: // AO
            return vec3(texValue.r); // Single channel
            
        case 5: // Emissive
            if ((texFlags & (1u << 5)) != 0u) { // TEXTURE_HDR
                return texValue.rgb * 4.0; // HDR scaling
            }
            return texValue.rgb;
            
        default:
            return texValue.rgb;
    }
}
```

---

## Debugging and Validation

### Debug Visualization in Shader

```glsl
// Debug mode uniform
uniform int u_DebugMode = 0; // 0=normal, 1=types, 2=flags, 3=handles

vec3 DebugVisualization(int index) {
    if (index < 0 || index >= textureData.types.length()) {
        return vec3(1.0, 0.0, 1.0); // Magenta for invalid index
    }
    
    switch (u_DebugMode) {
        case 1: // Visualize texture types
            uint texType = textureData.types[index];
            switch (texType) {
                case 0: return vec3(1.0, 0.0, 0.0); // Red for diffuse
                case 1: return vec3(0.0, 1.0, 0.0); // Green for normal
                case 2: return vec3(0.0, 0.0, 1.0); // Blue for metallic
                case 3: return vec3(1.0, 1.0, 0.0); // Yellow for roughness
                case 4: return vec3(0.0, 1.0, 1.0); // Cyan for AO
                case 5: return vec3(1.0, 0.0, 1.0); // Magenta for emissive
                default: return vec3(0.5);           // Gray for unknown
            }
            
        case 2: // Visualize flags
            uint flags = textureData.flags[index];
            float r = ((flags & 1u) != 0u) ? 1.0 : 0.0;      // Valid
            float g = ((flags & (1u << 4)) != 0u) ? 1.0 : 0.0; // sRGB
            float b = ((flags & (1u << 8)) != 0u) ? 1.0 : 0.0; // Mipmapped
            return vec3(r, g, b);
            
        case 3: // Visualize handle validity
            uint64_t handle = packUint2x32(textureData.handles[index]);
            return (handle != 0ul) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
            
        default:
            return SampleTexture(index, TexCoords).rgb;
    }
}
```

### C++ Debug Functions

```cpp
void DebugPrintTextureInfo() {
    std::cout << "=== Bindless Texture Debug Info ===" << std::endl;
    
    for (size_t i = 0; i < m_HandleData.size(); ++i) {
        const auto& data = m_HandleData[i];
        
        std::cout << "Texture [" << i << "]:" << std::endl;
        std::cout << "  Handle: 0x" << std::hex << data.handle << std::dec << std::endl;
        std::cout << "  Type: " << data.type << " (" << GetTypeString(data.type) << ")" << std::endl;
        std::cout << "  Flags: 0x" << std::hex << data.flags << std::dec << std::endl;
        std::cout << "  Valid: " << ((data.flags & 1) ? "YES" : "NO") << std::endl;
        
        if (data.flags & TEXTURE_SRGB) std::cout << "  - sRGB color space" << std::endl;
        if (data.flags & TEXTURE_HDR) std::cout << "  - HDR format" << std::endl;
        if (data.flags & TEXTURE_MIPMAPPED) std::cout << "  - Has mipmaps" << std::endl;
        if (data.flags & TEXTURE_COMPRESSED) std::cout << "  - Compressed" << std::endl;
        
        std::cout << std::endl;
    }
}

std::string GetTypeString(uint32_t type) {
    switch (type) {
        case 0: return "Diffuse";
        case 1: return "Normal";
        case 2: return "Metallic";
        case 3: return "Roughness";
        case 4: return "Ambient Occlusion";
        case 5: return "Emissive";
        case 6: return "Specular";
        case 7: return "Height";
        default: return "Unknown";
    }
}
```

### Validation Functions

```cpp
bool ValidateTextureData() {
    for (size_t i = 0; i < m_HandleData.size(); ++i) {
        const auto& data = m_HandleData[i];
        
        // Check handle validity
        if (data.handle == 0) {
            std::cerr << "Error: Texture [" << i << "] has null handle" << std::endl;
            return false;
        }
        
        // Check type range
        if (data.type > 7) {
            std::cerr << "Error: Texture [" << i << "] has invalid type: " << data.type << std::endl;
            return false;
        }
        
        // Check flag consistency
        if (!(data.flags & TEXTURE_VALID)) {
            std::cerr << "Warning: Texture [" << i << "] not marked as valid" << std::endl;
        }
        
        // Verify handle residency
        if (!IsHandleResident(data.handle)) {
            std::cerr << "Error: Texture [" << i << "] handle not resident" << std::endl;
            return false;
        }
    }
    
    return true;
}
```

---

## Performance Considerations

### Memory Usage

```cpp
// Memory usage calculation
size_t CalculateSSBOMemoryUsage(size_t textureCount) {
    size_t handleMemory = textureCount * sizeof(GLuint64);  // 8 bytes each
    size_t typeMemory = textureCount * sizeof(uint32_t);    // 4 bytes each
    size_t flagMemory = textureCount * sizeof(uint32_t);    // 4 bytes each
    
    size_t totalMemory = handleMemory + typeMemory + flagMemory; // 16 bytes per texture
    
    std::cout << "SSBO Memory Usage for " << textureCount << " textures:" << std::endl;
    std::cout << "  Handles: " << handleMemory << " bytes" << std::endl;
    std::cout << "  Types: " << typeMemory << " bytes" << std::endl;
    std::cout << "  Flags: " << flagMemory << " bytes" << std::endl;
    std::cout << "  Total: " << totalMemory << " bytes (" << (totalMemory / 1024.0f) << " KB)" << std::endl;
    
    return totalMemory;
}

// Example: 1024 textures = 16KB SSBO
```

### Optimization Tips

1. **Type-Based Sorting**: Group textures by type for better cache locality
2. **Flag-Based Culling**: Skip processing textures with error flags
3. **Batch Updates**: Minimize SSBO uploads by batching changes
4. **Smart Caching**: Cache frequently accessed type/flag combinations

---

## Summary

The texture types and flags system in your bindless implementation provides:

### **Texture Types Benefits:**
- ✅ Semantic clarity for material systems
- ✅ Standardized PBR workflow support
- ✅ Easy shader organization and maintenance
- ✅ Extensible for future material types

### **Texture Flags Benefits:**
- ✅ Runtime validation and error handling
- ✅ Format and capability information
- ✅ Performance optimization hints
- ✅ Debug and diagnostic support

### **Combined System Benefits:**
- ✅ Robust material pipeline with fallbacks
- ✅ Efficient GPU memory organization
- ✅ Comprehensive debugging capabilities
- ✅ Future-proof extensibility

This system enables sophisticated material workflows while maintaining performance and providing excellent debugging capabilities for production graphics applications.