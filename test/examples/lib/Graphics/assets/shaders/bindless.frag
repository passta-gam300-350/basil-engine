#version 450 core

// Enable bindless texture extension
#extension GL_ARB_bindless_texture : require

// Input from vertex shader
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Output
out vec4 FragColor;

// SSBO containing texture handles (must match C++ layout exactly)
layout(std430, binding = 1) readonly buffer TextureHandles {
    uvec2 handles[];        // Array of 64-bit handles (stored as 2x32-bit)
    uint types[];           // Array of texture types
    uint flags[];           // Array of texture flags
} textureData;

// Texture availability flags (set by bindless texture system)
uniform bool u_HasDiffuseMap;
uniform bool u_HasNormalMap;
uniform bool u_HasMetallicMap;
uniform bool u_HasRoughnessMap;
uniform bool u_HasAOMap;
uniform bool u_HasEmissiveMap;

// Texture indices for each type (set by bindless texture system)
uniform int u_DiffuseIndex;
uniform int u_NormalIndex;
uniform int u_MetallicIndex;
uniform int u_RoughnessIndex;
uniform int u_AOIndex;
uniform int u_EmissiveIndex;

// Lighting uniforms
uniform vec3 u_ViewPos;
uniform vec3 u_LightPos;
uniform vec3 u_LightColor;

// Material properties (fallback when no textures available)
uniform vec3 u_AlbedoColor = vec3(0.8, 0.8, 0.8);
uniform float u_MetallicValue = 0.0;
uniform float u_RoughnessValue = 0.5;

// Helper function to sample texture using bindless handle
vec4 SampleTexture(int index, vec2 coords) {
    if (index >= 0 && index < textureData.handles.length()) {
        // Reconstruct 64-bit handle from two 32-bit values
        uint64_t handle = packUint2x32(textureData.handles[index]);
        
        // Check if handle is valid (flags bit 0)
        if ((textureData.flags[index] & 1u) != 0u && handle != 0ul) {
            // Cast handle to sampler2D and sample
            return texture(sampler2D(handle), coords);
        }
    }
    return vec4(1.0); // Default white
}

// Normal mapping helper
vec3 SampleNormal(int index, vec2 coords, vec3 normal, vec3 tangent) {
    if (index >= 0 && index < textureData.handles.length()) {
        uint64_t handle = packUint2x32(textureData.handles[index]);
        
        if ((textureData.flags[index] & 1u) != 0u && handle != 0ul) {
            vec3 normalMap = texture(sampler2D(handle), coords).xyz * 2.0 - 1.0;
            
            // Create TBN matrix
            vec3 N = normalize(normal);
            vec3 T = normalize(tangent - dot(tangent, N) * N);
            vec3 B = cross(N, T);
            mat3 TBN = mat3(T, B, N);
            
            return normalize(TBN * normalMap);
        }
    }
    return normalize(normal);
}

void main() {
    // Sample textures using bindless handles
    vec3 albedo = u_AlbedoColor;
    if (u_HasDiffuseMap) {
        albedo = SampleTexture(u_DiffuseIndex, TexCoords).rgb;
    }
    
    float metallic = u_MetallicValue;
    if (u_HasMetallicMap) {
        metallic = SampleTexture(u_MetallicIndex, TexCoords).r;
    }
    
    float roughness = u_RoughnessValue;
    if (u_HasRoughnessMap) {
        roughness = SampleTexture(u_RoughnessIndex, TexCoords).r;
    }
    
    float ao = 1.0;
    if (u_HasAOMap) {
        ao = SampleTexture(u_AOIndex, TexCoords).r;
    }
    
    vec3 emissive = vec3(0.0);
    if (u_HasEmissiveMap) {
        emissive = SampleTexture(u_EmissiveIndex, TexCoords).rgb;
    }
    
    // Normal mapping (simplified - would need tangent space for proper normal mapping)
    vec3 normal = normalize(Normal);
    if (u_HasNormalMap) {
        // For proper normal mapping, you'd need tangent vectors
        // This is a simplified version
        vec3 normalMap = SampleTexture(u_NormalIndex, TexCoords).xyz * 2.0 - 1.0;
        normal = normalize(normal + normalMap * 0.1); // Simple perturbation
    }
    
    // Simplified PBR lighting calculation
    vec3 viewDir = normalize(u_ViewPos - FragPos);
    vec3 lightDir = normalize(u_LightPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    
    // Ambient
    vec3 ambient = 0.15 * albedo * ao;
    
    // Diffuse
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = NdotL * u_LightColor * albedo * (1.0 - metallic);
    
    // Specular (simplified)
    float NdotH = max(dot(normal, halfwayDir), 0.0);
    float spec = pow(NdotH, 32.0 * (1.0 - roughness));
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 specular = spec * F0 * u_LightColor;
    
    // Final color
    vec3 color = ambient + diffuse + specular + emissive;
    
    // Simple tone mapping
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2)); // Gamma correction
    
    FragColor = vec4(color, 1.0);
}