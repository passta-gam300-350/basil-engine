#version 450 core

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

// Traditional texture uniforms - using #define texture slots
uniform sampler2D u_DiffuseMap;    // Slot 0
uniform sampler2D u_NormalMap;     // Slot 1
uniform sampler2D u_MetallicMap;   // Slot 2
uniform sampler2D u_RoughnessMap;  // Slot 3
uniform sampler2D u_AOMap;         // Slot 4
uniform sampler2D u_EmissiveMap;   // Slot 5
uniform sampler2D u_SpecularMap;   // Slot 6
uniform sampler2D u_HeightMap;     // Slot 7
uniform sampler2D u_ShadowMap;     // Slot 8

// Point shadow cubemaps (slots 9-12 for up to 4 point lights)
uniform samplerCube u_PointShadowMaps[4];
uniform float u_PointShadowFarPlanes[4];
uniform int u_NumPointShadowMaps = 0;

// Directional shadow maps (currently only 1 supported, but prepared for expansion)
uniform mat4 u_LightSpaceMatrix;  // Light space matrix for directional shadow

// Texture availability flags (set by texture slot system)
uniform bool u_HasDiffuseMap = false;
uniform bool u_HasNormalMap = false;
uniform bool u_HasMetallicMap = false;
uniform bool u_HasRoughnessMap = false;
uniform bool u_HasAOMap = false;
uniform bool u_HasEmissiveMap = false;
uniform bool u_HasSpecularMap = false;
uniform bool u_HasHeightMap = false;
uniform bool u_EnableShadows = false;

// Shadow intensity controls (0.0 = no shadow, 1.0 = full shadow)
uniform float u_DirectionalShadowIntensity = 0.8;
uniform float u_PointShadowIntensity = 0.8;

// Multi-light system for instanced rendering
struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;           // Diffuse intensity (ogldev-style)
    float ambientIntensity;    // Per-light ambient contribution (ogldev-style)
    float constant;
    float linear;
    float quadratic;
};

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;           // Diffuse intensity (ogldev-style)
    float ambientIntensity;    // Per-light ambient contribution (ogldev-style)
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;           // Diffuse intensity (ogldev-style)
    float ambientIntensity;    // Per-light ambient contribution (ogldev-style)
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

// Material properties (fallback when no textures available)
uniform vec3 u_AlbedoColor = vec3(0.8, 0.8, 0.8);
uniform float u_MetallicValue = 0.0;
uniform float u_RoughnessValue = 0.5;

// Ambient lighting
uniform vec3 u_AmbientLight = vec3(0.03);

// PBR constants
const float PI = 3.14159265359;

// Directional shadow calculation - now takes fragment position and calculates light space internally
float DirectionalShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir)
{
    // Only calculate shadow if enabled
    if (!u_EnableShadows) {
        return 0.0;
    }

    // Transform fragment position to light space
    vec4 fragPosLightSpace = u_LightSpaceMatrix * vec4(fragPos, 1.0);

    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Check if we're outside the shadow map
    if(projCoords.z > 1.0)
        return 0.0;

    // Get closest depth value from light's perspective
    float closestDepth = texture(u_ShadowMap, projCoords.xy).r;

    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // Calculate bias based on surface angle to light (reduces shadow acne)
    vec3 N = normalize(normal);
    vec3 L = normalize(-lightDir);
    float bias = max(0.005 * (1.0 - dot(N, L)), 0.001);

    // PCF (Percentage Closer Filtering) for softer shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

// Point shadow calculation using cubemap
// Samples from array of up to 4 samplerCubes
float PointShadowCalculation(vec3 fragPos, vec3 lightPos, int lightIndex, float farPlane)
{
    // Check if we have a shadow map for this light
    if (lightIndex >= u_NumPointShadowMaps) {
        return 0.0; // No shadow
    }

    // Get vector from light to fragment
    vec3 fragToLight = fragPos - lightPos;

    // Sample cubemap depth (stored as distance/farPlane)
    float closestDepth = texture(u_PointShadowMaps[lightIndex], fragToLight).r;

    // Convert back to original distance
    closestDepth *= farPlane;

    // Get current distance
    float currentDepth = length(fragToLight);

    // Calculate bias based on surface angle
    float bias = 0.05;

    // PCF filtering for softer shadows
    float shadow = 0.0;
    float samples = 4.0;
    float offset = 0.1;
    for(float x = -offset; x < offset; x += offset / (samples * 0.5))
    {
        for(float y = -offset; y < offset; y += offset / (samples * 0.5))
        {
            for(float z = -offset; z < offset; z += offset / (samples * 0.5))
            {
                float pcfDepth = texture(u_PointShadowMaps[lightIndex], fragToLight + vec3(x, y, z)).r;
                pcfDepth *= farPlane;
                shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
            }
        }
    }
    shadow /= (samples * samples * samples);

    return shadow;
}

// Enhanced normal mapping helper for traditional textures
vec3 getNormalFromMap() {
    if (!u_HasNormalMap) {
        return normalize(fs_in.Normal);
    }

    vec3 tangentNormal = texture(u_NormalMap, fs_in.TexCoords).xyz * 2.0 - 1.0;
    return normalize(fs_in.TBN * tangentNormal);
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
vec3 calculatePointLight(PointLight light, vec3 albedo, vec3 normal, vec3 fragPos, vec3 viewDir, float metallic, float roughness, vec3 F0, int lightIndex)
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

    // Calculate point shadow
    float shadowFactor = 0.0;
    if (lightIndex < u_NumPointShadowMaps) {
        shadowFactor = PointShadowCalculation(fragPos, light.position, lightIndex, u_PointShadowFarPlanes[lightIndex]);
    }

    // Apply shadow to lighting contribution with configurable intensity
    vec3 lightContribution = (kD * albedo / PI + specular) * radiance * NdotL;
    return lightContribution * (1.0 - shadowFactor * u_PointShadowIntensity);
}

// Calculate contribution from a directional light
vec3 calculateDirectionalLight(DirectionalLight light, vec3 albedo, vec3 normal, vec3 fragPos, vec3 viewDir, float metallic, float roughness, vec3 F0, int lightIndex)
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

    // Calculate directional shadow (currently only supports first directional light)
    float shadowFactor = 0.0;
    if (lightIndex == 0) {  // Only first directional light has shadow map
        shadowFactor = DirectionalShadowCalculation(fragPos, normal, light.direction);
    }

    // Apply shadow to directional light contribution with configurable intensity
    vec3 lightContribution = (kD * albedo / PI + specular) * radiance * NdotL;
    return lightContribution * (1.0 - shadowFactor * u_DirectionalShadowIntensity);
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

// Multi-light PBR lighting calculation (ogldev-style with per-light ambient)
vec3 calculateMultiLightPBR(vec3 albedo, vec3 normal, float metallic, float roughness, float ao)
{
    vec3 N = normalize(normal);
    vec3 V = normalize(u_ViewPos - fs_in.FragPos);

    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Accumulate lighting contribution (direct + specular)
    vec3 Lo = vec3(0.0);

    // Accumulate per-light ambient contribution (ogldev-style)
    vec3 totalAmbient = vec3(0.0);

    // Point lights (with individual shadows via cubemaps)
    for (int i = 0; i < u_NumPointLights && i < 8; ++i) {
        // Direct lighting
        Lo += calculatePointLight(u_PointLights[i], albedo, normal, fs_in.FragPos, V, metallic, roughness, F0, i);

        // Per-light ambient (ogldev-style: attenuated by distance)
        float distance = length(u_PointLights[i].position - fs_in.FragPos);
        float attenuation = 1.0 / (u_PointLights[i].constant + u_PointLights[i].linear * distance +
                                   u_PointLights[i].quadratic * (distance * distance));
        totalAmbient += u_PointLights[i].color * u_PointLights[i].ambientIntensity * attenuation * albedo * ao;
    }

    // Directional lights (with individual shadows)
    for (int i = 0; i < u_NumDirectionalLights && i < 4; ++i) {
        // Direct lighting
        Lo += calculateDirectionalLight(u_DirectionalLights[i], albedo, normal, fs_in.FragPos, V, metallic, roughness, F0, i);

        // Per-light ambient (ogldev-style: no attenuation for directional)
        totalAmbient += u_DirectionalLights[i].color * u_DirectionalLights[i].ambientIntensity * albedo * ao;
    }

    // Spot lights (no shadows for now)
    for (int i = 0; i < u_NumSpotLights && i < 4; ++i) {
        // Direct lighting
        Lo += calculateSpotLight(u_SpotLights[i], albedo, normal, fs_in.FragPos, V, metallic, roughness, F0);

        // Per-light ambient (ogldev-style: attenuated by distance)
        float distance = length(u_SpotLights[i].position - fs_in.FragPos);
        float attenuation = 1.0 / (u_SpotLights[i].constant + u_SpotLights[i].linear * distance +
                                   u_SpotLights[i].quadratic * (distance * distance));
        totalAmbient += u_SpotLights[i].color * u_SpotLights[i].ambientIntensity * attenuation * albedo * ao;
    }

    // Add global ambient as fallback (can be set to 0 if using only per-light ambient)
    totalAmbient += u_AmbientLight * albedo * ao;

    return totalAmbient + Lo;
}

void main() {
    // Sample traditional textures
    vec3 albedo = fs_in.InstanceColor.rgb;
    if (u_HasDiffuseMap) {
        vec4 texSample = texture(u_DiffuseMap, fs_in.TexCoords);
        albedo *= texSample.rgb;
    }

    float metallic = fs_in.InstanceMetallic;
    if (u_HasMetallicMap) {
        metallic = texture(u_MetallicMap, fs_in.TexCoords).r;
    }

    float roughness = fs_in.InstanceRoughness;
    if (u_HasRoughnessMap) {
        roughness = texture(u_RoughnessMap, fs_in.TexCoords).r;
    }

    float ao = 1.0;
    if (u_HasAOMap) {
        ao = texture(u_AOMap, fs_in.TexCoords).r;
    }

    vec3 emissive = vec3(0.0);
    if (u_HasEmissiveMap) {
        emissive = texture(u_EmissiveMap, fs_in.TexCoords).rgb;
    }

    // Get normal from traditional normal map
    vec3 normal = getNormalFromMap();

    // Calculate multi-light PBR lighting (shadows are calculated per-light now)
    vec3 color = calculateMultiLightPBR(albedo, normal, metallic, roughness, ao);

    // Add emissive
    color += emissive;

    // Output raw HDR color to HDR framebuffer (RGB16F)
    // Tone mapping will be applied later in ToneMapRenderPass
    FragColor = vec4(color, 1.0);
}