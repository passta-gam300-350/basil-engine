#version 450 core

// Enable bindless texture extension
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_gpu_shader_int64 : enable

// Input from vertex shader
in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;

    // Shadow mapping
    vec4 FragPosLightSpace;

    // Per-instance material data from vertex shader
    vec4 InstanceColor;
    float InstanceMetallic;
    float InstanceRoughness;
} fs_in;

// Output
out vec4 FragColor;

// Bindless texture handles SSBO (binding = 1, separate from instance matrices at binding = 0)
layout(std430, binding = 1) readonly buffer TextureHandles {
    uvec2 handles[1024];        // 64-bit handles as 2×32-bit values
    uint types[1024];           // Texture semantic types
    uint flags[1024];           // Texture metadata flags
} textureData;

// Texture availability flags (set by bindless texture system)
uniform bool u_HasDiffuseMap = false;
uniform bool u_HasNormalMap = false;
uniform bool u_HasMetallicMap = false;
uniform bool u_HasRoughnessMap = false;
uniform bool u_HasAOMap = false;
uniform bool u_HasEmissiveMap = false;

// Texture indices for each type (set by bindless texture system)
uniform int u_DiffuseIndex = -1;
uniform int u_NormalIndex = -1;
uniform int u_MetallicIndex = -1;
uniform int u_RoughnessIndex = -1;
uniform int u_AOIndex = -1;
uniform int u_EmissiveIndex = -1;

// Multi-light system for instanced rendering
struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
};

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
};

// Light uniforms
uniform int u_NumPointLights = 4;
uniform int u_NumDirectionalLights = 1;
uniform int u_NumSpotLights = 2;

uniform PointLight u_PointLights[8];
uniform DirectionalLight u_DirectionalLights[4];
uniform SpotLight u_SpotLights[4];

// Camera
uniform vec3 u_ViewPos;

// Shadow mapping
uniform sampler2D u_ShadowMap;

// Material properties (fallback when no textures available)
uniform vec3 u_AlbedoColor = vec3(0.8, 0.8, 0.8);
uniform float u_MetallicValue = 0.0;
uniform float u_RoughnessValue = 0.5;

// PBR constants
const float PI = 3.14159265359;

// Shadow mapping function
float ShadowCalculation(vec4 fragPosLightSpace)
{
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Check if we're outside the shadow map
    if(projCoords.z > 1.0)
        return 0.0;

    // Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(u_ShadowMap, projCoords.xy).r;

    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // Calculate bias to reduce shadow acne
    float bias = 0.005;

    // Check whether current frag pos is in shadow with PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

// Helper function to sample bindless texture safely
vec4 SampleTexture(int index, vec2 coords) {
    if (index >= 0 && index < textureData.handles.length()) {
        if ((textureData.flags[index] & 1u) != 0u) {
            // Reconstruct 64-bit handle from two 32-bit parts
            uvec2 handleParts = textureData.handles[index];
            return texture(sampler2D(handleParts), coords);
        }
    }
    return vec4(1.0); // Default white
}

// Enhanced normal mapping helper for bindless textures
vec3 getNormalFromMap() {
    if (!u_HasNormalMap || u_NormalIndex < 0) {
        return normalize(fs_in.Normal);
    }
    
    if (u_NormalIndex < textureData.handles.length()) {
        if ((textureData.flags[u_NormalIndex] & 1u) != 0u) {
            uint64_t handle = packUint2x32(textureData.handles[u_NormalIndex]);
            vec3 tangentNormal = texture(sampler2D(handle), fs_in.TexCoords).xyz * 2.0 - 1.0;
            return normalize(fs_in.TBN * tangentNormal);
        }
    }
    return normalize(fs_in.Normal);
}

// PBR Distribution function (GGX/Trowbridge-Reitz)
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

// PBR Geometry function (Smith's method)
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// PBR Fresnel function (Schlick's approximation)
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Calculate contribution from a single point light
vec3 calculatePointLight(PointLight light, vec3 albedo, vec3 normal, vec3 fragPos, vec3 viewDir, float metallic, float roughness, vec3 F0)
{
    vec3 N = normalize(normal);
    vec3 L = normalize(light.position - fragPos);
    vec3 H = normalize(viewDir + L);
    
    // Calculate attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    vec3 radiance = light.color * light.intensity * attenuation;
    
    // Calculate Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G   = geometrySmith(N, viewDir, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    float NdotL = max(dot(N, L), 0.0);
    
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, viewDir), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;
    
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// Calculate contribution from a directional light
vec3 calculateDirectionalLight(DirectionalLight light, vec3 albedo, vec3 normal, vec3 viewDir, float metallic, float roughness, vec3 F0)
{
    vec3 N = normalize(normal);
    vec3 L = normalize(-light.direction);
    vec3 H = normalize(viewDir + L);
    
    vec3 radiance = light.color * light.intensity;
    
    // Calculate Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G   = geometrySmith(N, viewDir, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    float NdotL = max(dot(N, L), 0.0);
    
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, viewDir), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;
    
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// Calculate contribution from a spot light
vec3 calculateSpotLight(SpotLight light, vec3 albedo, vec3 normal, vec3 fragPos, vec3 viewDir, float metallic, float roughness, vec3 F0)
{
    vec3 N = normalize(normal);
    vec3 L = normalize(light.position - fragPos);
    vec3 H = normalize(viewDir + L);
    
    // Calculate attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    // Calculate spot light intensity
    float theta = dot(L, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    vec3 radiance = light.color * light.intensity * attenuation * intensity;
    
    // Calculate Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G   = geometrySmith(N, viewDir, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    float NdotL = max(dot(N, L), 0.0);
    
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, viewDir), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;
    
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// Multi-light PBR lighting calculation
vec3 calculateMultiLightPBR(vec3 albedo, vec3 normal, float metallic, float roughness, float ao)
{
    vec3 N = normalize(normal);
    vec3 V = normalize(u_ViewPos - fs_in.FragPos);
    
    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Accumulate lighting contribution
    vec3 Lo = vec3(0.0);
    
    // Point lights
    for (int i = 0; i < u_NumPointLights && i < 8; ++i) {
        Lo += calculatePointLight(u_PointLights[i], albedo, normal, fs_in.FragPos, V, metallic, roughness, F0);
    }
    
    // Directional lights
    for (int i = 0; i < u_NumDirectionalLights && i < 4; ++i) {
        Lo += calculateDirectionalLight(u_DirectionalLights[i], albedo, normal, V, metallic, roughness, F0);
    }
    
    // Spot lights
    for (int i = 0; i < u_NumSpotLights && i < 4; ++i) {
        Lo += calculateSpotLight(u_SpotLights[i], albedo, normal, fs_in.FragPos, V, metallic, roughness, F0);
    }
    
    // Ambient lighting
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    return ambient + Lo;
}

void main() {
    // Sample bindless textures using handles
    vec3 albedo = fs_in.InstanceColor.rgb;
    if (u_HasDiffuseMap && u_DiffuseIndex >= 0) {
        albedo *= SampleTexture(u_DiffuseIndex, fs_in.TexCoords).rgb;
    }
    
    float metallic = fs_in.InstanceMetallic;
    if (u_HasMetallicMap && u_MetallicIndex >= 0) {
        metallic = SampleTexture(u_MetallicIndex, fs_in.TexCoords).r;
    }
    
    float roughness = fs_in.InstanceRoughness;
    if (u_HasRoughnessMap && u_RoughnessIndex >= 0) {
        roughness = SampleTexture(u_RoughnessIndex, fs_in.TexCoords).r;
    }
    
    float ao = 1.0;
    if (u_HasAOMap && u_AOIndex >= 0) {
        ao = SampleTexture(u_AOIndex, fs_in.TexCoords).r;
    }
    
    vec3 emissive = vec3(0.0);
    if (u_HasEmissiveMap && u_EmissiveIndex >= 0) {
        emissive = SampleTexture(u_EmissiveIndex, fs_in.TexCoords).rgb;
    }
    
    // Get normal from bindless normal map
    vec3 normal = getNormalFromMap();
    
    // Calculate shadow factor
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace);

    // Calculate multi-light PBR lighting
    vec3 color = calculateMultiLightPBR(albedo, normal, metallic, roughness, ao);

    // Apply shadows (reduce lighting by shadow factor)
    // Note: This is a simplified approach - ideally shadows should only affect direct lighting
    color = color * (1.0 - shadow * 0.7); // Reduce shadow intensity to 70%

    // Add emissive
    color += emissive;
    
    // Tone mapping and gamma correction
    color = color / (color + vec3(1.0)); // Reinhard tone mapping
    color = pow(color, vec3(1.0/2.2));   // Gamma correction
    
    FragColor = vec4(color, 1.0);
}