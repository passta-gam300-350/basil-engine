#version 450 core

// Input from vertex shader
in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;

    // Per-instance material data from vertex shader
    vec4 InstanceColor;
    float InstanceMetallic;
    float InstanceRoughness;
    float InstanceNormalStrength;
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

// ===== UNIFIED SHADOW TEXTURE SYSTEM (TEXTURE ARRAYS) =====
// Texture array for 2D shadows (directional/spot) - UNLIMITED shadows!
uniform sampler2DArray u_Shadow2DTextureArray;   // Layered texture array - slot 8
// Cubemap array for point shadows (can support more, but keeping individual for now)
uniform samplerCube u_ShadowCubemapTextures[8];  // Cubemap depth maps (point) - slots 24-31

// Texture availability flags (set by texture slot system)
uniform bool u_HasDiffuseMap = false;
uniform bool u_HasNormalMap = false;
uniform bool u_HasMetallicMap = false;
uniform bool u_HasRoughnessMap = false;
uniform bool u_HasAOMap = false;
uniform bool u_HasEmissiveMap = false;
uniform bool u_HasSpecularMap = false;
uniform bool u_HasHeightMap = false;

// ===== UNIFIED LIGHT SYSTEM (SSBO-BASED, UNLIMITED LIGHTS) =====
// Unified light data structure (must match C++ LightData struct)
struct LightData {
    int type;               // 4 bytes - 0=directional, 1=point, 2=spot
    float _pad0;            // 4 bytes - padding for alignment
    float _pad1;            // 4 bytes - padding for alignment
    float _pad2;            // 4 bytes - padding for alignment
    vec3 position;          // 12 bytes
    float intensity;        // 4 bytes - Diffuse intensity (ogldev-style)
    vec3 direction;         // 12 bytes
    float ambientIntensity; // 4 bytes - Per-light ambient contribution
    vec3 color;             // 12 bytes
    float cutOff;           // 4 bytes - Spot inner cutoff (cos angle)
    float outerCutOff;      // 4 bytes - Spot outer cutoff (cos angle)
    float constant;         // 4 bytes - Attenuation constant
    float linear;           // 4 bytes - Attenuation linear
    float quadratic;        // 4 bytes - Attenuation quadratic
    // Total: 80 bytes per light (std430 aligned)
};

// Light types (must match C++ enum)
const int LIGHT_TYPE_DIRECTIONAL = 0;
const int LIGHT_TYPE_POINT = 1;
const int LIGHT_TYPE_SPOT = 2;

// SSBO containing all lights (binding = 3)
layout(std430, binding = 3) buffer LightBuffer {
    LightData lights[];
};

// Number of active lights in the SSBO
uniform int u_NumLights = 0;

// Camera
uniform vec3 u_ViewPos;

// Material properties (fallback when no textures available)
uniform vec3 u_AlbedoColor = vec3(0.8, 0.8, 0.8);
uniform float u_MetallicValue = 0.0;
uniform float u_RoughnessValue = 0.5;

// Ambient lighting
uniform vec3 u_AmbientLight = vec3(0.03);

// Fog uniforms (OGLDev Tutorial 39-style)
uniform int u_FogType = 0;                    // 0=None, 1=Linear, 2=Exp, 3=ExpSquared
uniform float u_FogStart = 10.0;              // Linear fog start distance
uniform float u_FogEnd = 50.0;                // Fog end distance (all types)
uniform float u_FogDensity = 0.05;            // Exponential fog density
uniform vec3 u_FogColor = vec3(0.5, 0.5, 0.5); // Fog color

// Blend mode control (true = opaque pass, false = transparent pass)
uniform bool u_IsOpaquePass = true;

// PBR constants
const float PI = 3.14159265359;

// ===== UNIFIED SHADOW SYSTEM (SSBO-BASED) =====
// Shadow data structure (must match C++ ShadowData struct)
struct ShadowData {
    mat4 lightSpaceMatrix;     // 64 bytes - For directional/spot
    int shadowType;            // 4 bytes - 0=none, 1=directional, 2=point, 3=spot
    int textureIndex;          // 4 bytes - Index into shadow texture arrays
    float farPlane;            // 4 bytes - For point lights
    float intensity;           // 4 bytes - Shadow intensity (0.0-1.0)
    // Total: 80 bytes
};

// Shadow types
const int SHADOW_TYPE_NONE = 0;
const int SHADOW_TYPE_DIRECTIONAL = 1;
const int SHADOW_TYPE_POINT = 2;
const int SHADOW_TYPE_SPOT = 3;

// SSBO containing all shadow data (binding = 1)
layout(std430, binding = 1) buffer ShadowBuffer {
    ShadowData shadows[];
};

// Number of active shadows in the SSBO
uniform int u_NumShadows = 0;

// ===== RANDOM SHADOW SAMPLING CONFIGURATION =====
uniform sampler3D u_ShadowMapOffsetTexture;  // 3D texture with Poisson disk offsets
uniform int u_ShadowMapFilterSize = 8;       // Filter size (8 = 8x8 = 64 samples)
uniform int u_ShadowMapOffsetTextureSize = 16; // Tiling pattern size
uniform float u_ShadowMapRandomRadius = 15.0;  // Shadow softness control

// Shadow texture arrays (indexed by shadowData.textureIndex)
// We keep individual slots for now to avoid dynamic indexing issues
// Slots 8-39 reserved for shadows (32 total shadow maps)

// ===== UNIFIED SSBO-BASED SHADOW CALCULATION FUNCTIONS =====
// These functions read shadow data from the SSBO and calculate shadows dynamically

// ===== RANDOM SHADOW SAMPLING WITH ADAPTIVE QUALITY =====
// Uses Poisson disk distribution for natural soft shadows without banding
float Calculate2DShadow(vec3 fragPos, vec3 normal, vec3 lightDir, mat4 lightSpaceMatrix, int textureIndex)
{
    // Transform fragment position to light space
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Discard fragments beyond light frustum
    if(projCoords.z > 1.0) return 0.0;

    // Adaptive bias based on surface angle (reduces shadow acne)
    vec3 N = normalize(normal);
    vec3 L = normalize(-lightDir);
    float bias = max(0.005 * (1.0 - dot(N, L)), 0.001);

    // Get texture coordinates for offset lookup (screen-space tiling)
    ivec3 offsetCoord;
    vec2 f = mod(gl_FragCoord.xy, vec2(u_ShadowMapOffsetTextureSize));
    offsetCoord.yz = ivec2(f);

    // Get texel size for shadow map sampling (use texture array size)
    vec2 texelSize = 1.0 / vec2(textureSize(u_Shadow2DTextureArray, 0).xy);

    float currentDepth = projCoords.z;
    float shadow = 0.0;

    // ADAPTIVE SAMPLING PHASE 1: Quick 8 samples to detect full shadow/light
    // This phase runs for ALL pixels (fully lit, fully shadowed, and penumbra)
    int samplesDiv2 = (u_ShadowMapFilterSize * u_ShadowMapFilterSize) / 2;

    for (int i = 0; i < 4; i++) {
        offsetCoord.x = i;
        // Fetch 4 offset values from 3D texture (RGBA = 2 offset pairs)
        vec4 offsets = texelFetch(u_ShadowMapOffsetTexture, offsetCoord, 0) * u_ShadowMapRandomRadius;

        // Sample 1: Use RG channels as offset (use vec3 for texture array: uv + layer)
        vec2 sampleCoord = projCoords.xy + offsets.rg * texelSize;
        float depth = texture(u_Shadow2DTextureArray, vec3(sampleCoord, textureIndex)).r;
        shadow += (currentDepth - bias > depth) ? 1.0 : 0.0;

        // Sample 2: Use BA channels as offset
        sampleCoord = projCoords.xy + offsets.ba * texelSize;
        depth = texture(u_Shadow2DTextureArray, vec3(sampleCoord, textureIndex)).r;
        shadow += (currentDepth - bias > depth) ? 1.0 : 0.0;
    }

    float shadowFactor = shadow / 8.0;

    // ADAPTIVE SAMPLING PHASE 2: Additional samples only for penumbra regions
    // This phase is SKIPPED for fully lit (shadowFactor = 0) and fully shadowed (shadowFactor = 1) pixels
    // Provides massive performance boost in non-penumbra areas
    if (shadowFactor > 0.0 && shadowFactor < 1.0) {
        // We're in the penumbra (soft shadow edge) - continue sampling for better quality
        for (int i = 4; i < samplesDiv2; i++) {
            offsetCoord.x = i;
            vec4 offsets = texelFetch(u_ShadowMapOffsetTexture, offsetCoord, 0) * u_ShadowMapRandomRadius;

            // Sample 1: RG channels (use vec3 for texture array: uv + layer)
            vec2 sampleCoord = projCoords.xy + offsets.rg * texelSize;
            float depth = texture(u_Shadow2DTextureArray, vec3(sampleCoord, textureIndex)).r;
            shadow += (currentDepth - bias > depth) ? 1.0 : 0.0;

            // Sample 2: BA channels
            sampleCoord = projCoords.xy + offsets.ba * texelSize;
            depth = texture(u_Shadow2DTextureArray, vec3(sampleCoord, textureIndex)).r;
            shadow += (currentDepth - bias > depth) ? 1.0 : 0.0;
        }

        // Recalculate shadow factor with all samples
        shadowFactor = shadow / float(samplesDiv2 * 2);
    }

    return shadowFactor;
}

// Calculate cubemap shadow (point) using unified texture array
float CalculateCubemapShadow(vec3 fragPos, vec3 lightPos, int textureIndex, float farPlane)
{
    vec3 fragToLight = fragPos - lightPos;
    float closestDepth = texture(u_ShadowCubemapTextures[textureIndex], fragToLight).r;
    closestDepth *= farPlane;
    float currentDepth = length(fragToLight);
    float bias = 0.05;

    // PCF filtering
    float shadow = 0.0;
    float samples = 4.0;
    float offset = 0.1;
    for(float x = -offset; x < offset; x += offset / (samples * 0.5)) {
        for(float y = -offset; y < offset; y += offset / (samples * 0.5)) {
            for(float z = -offset; z < offset; z += offset / (samples * 0.5)) {
                float pcfDepth = texture(u_ShadowCubemapTextures[textureIndex], fragToLight + vec3(x, y, z)).r;
                pcfDepth *= farPlane;
                shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
            }
        }
    }
    shadow /= (samples * samples * samples);
    return shadow;
}

// Find shadow index for a given light type and light index
// Returns -1 if no shadow found
int FindShadowIndex(int lightType, int lightIndex)
{
    int typeCount = 0;
    for (int i = 0; i < u_NumShadows; ++i) {
        if (shadows[i].shadowType == lightType) {
            if (typeCount == lightIndex) {
                return i;  // Found the shadow for this light
            }
            typeCount++;
        }
    }
    return -1;  // No shadow found
}

// Calculate shadow for a given light index using SSBO data
// Returns shadow factor (0.0 = no shadow, 1.0 = full shadow)
float CalculateShadowFromSSBO(int shadowIndex, vec3 fragPos, vec3 normal, vec3 lightPos, vec3 lightDir)
{
    // Check if shadow index is valid
    if (shadowIndex < 0 || shadowIndex >= u_NumShadows) {
        return 0.0;  // No shadow
    }

    ShadowData shadowData = shadows[shadowIndex];

    // Check shadow type and dispatch to appropriate function
    if (shadowData.shadowType == SHADOW_TYPE_NONE) {
        return 0.0;
    }
    else if (shadowData.shadowType == SHADOW_TYPE_DIRECTIONAL || shadowData.shadowType == SHADOW_TYPE_SPOT) {
        // Use unified 2D shadow calculation
        return Calculate2DShadow(fragPos, normal, lightDir, shadowData.lightSpaceMatrix, shadowData.textureIndex) * shadowData.intensity;
    }
    else if (shadowData.shadowType == SHADOW_TYPE_POINT) {
        // Use unified cubemap shadow calculation
        return CalculateCubemapShadow(fragPos, lightPos, shadowData.textureIndex, shadowData.farPlane) * shadowData.intensity;
    }

    return 0.0;
}

// Enhanced normal mapping helper with BC5 format support
vec3 getNormalFromMap() {
    if (!u_HasNormalMap) {
        return normalize(fs_in.Normal);
    }

    // Sample normal map (BC5 format stores only RG channels)
    vec3 tangentNormal = texture(u_NormalMap, fs_in.TexCoords).xyz;

    // Unpack RG from [0,1] to [-1,1]
    tangentNormal.xy = tangentNormal.xy * 2.0 - 1.0;

    // Apply per-instance normal strength (bump scale)
    tangentNormal.xy *= fs_in.InstanceNormalStrength;

    // Reconstruct Z component for BC5/2-channel normal maps
    // Since normals are unit vectors: x² + y² + z² = 1
    // Therefore: z = sqrt(1 - x² - y²)
    tangentNormal.z = sqrt(max(1.0 - dot(tangentNormal.xy, tangentNormal.xy), 0.0));

    // Transform from tangent space to world space
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

// Calculate contribution from a point light (using unified SSBO)
vec3 calculatePointLight(LightData light, vec3 albedo, vec3 normal, vec3 fragPos, vec3 viewDir, float metallic, float roughness, vec3 F0, int lightIndex)
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

    // Calculate point shadow using SSBO
    int shadowIndex = FindShadowIndex(SHADOW_TYPE_POINT, lightIndex);
    float shadowFactor = CalculateShadowFromSSBO(shadowIndex, fragPos, normal, light.position, vec3(0.0));

    // Apply shadow to lighting contribution
    vec3 lightContribution = (kD * albedo / PI + specular) * radiance * NdotL;
    return lightContribution * (1.0 - shadowFactor);
}

// Calculate contribution from a directional light (using unified SSBO)
vec3 calculateDirectionalLight(LightData light, vec3 albedo, vec3 normal, vec3 fragPos, vec3 viewDir, float metallic, float roughness, vec3 F0, int lightIndex)
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

    // Calculate directional shadow using SSBO
    int shadowIndex = FindShadowIndex(SHADOW_TYPE_DIRECTIONAL, lightIndex);
    float shadowFactor = CalculateShadowFromSSBO(shadowIndex, fragPos, normal, vec3(0.0), light.direction);

    // Apply shadow to directional light contribution
    vec3 lightContribution = (kD * albedo / PI + specular) * radiance * NdotL;
    return lightContribution * (1.0 - shadowFactor);
}

// Calculate contribution from a spot light (using unified SSBO, supports shadows)
vec3 calculateSpotLight(LightData light, vec3 albedo, vec3 normal, vec3 fragPos, vec3 viewDir, float metallic, float roughness, vec3 F0, int lightIndex)
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

    // Calculate spot shadow using SSBO
    vec3 lightDir = normalize(fragPos - light.position);
    int shadowIndex = FindShadowIndex(SHADOW_TYPE_SPOT, lightIndex);
    float shadowFactor = CalculateShadowFromSSBO(shadowIndex, fragPos, normal, light.position, lightDir);

    // Apply shadow to diffuse and specular
    vec3 lightContribution = (kD * albedo / PI + specular) * radiance * NdotL;
    return lightContribution * (1.0 - shadowFactor);
}

// Multi-light PBR lighting calculation (unified SSBO, ogldev-style with per-light ambient)
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

    // Track light type indices for shadow mapping
    int directionalLightIndex = 0;
    int pointLightIndex = 0;
    int spotLightIndex = 0;

    // Iterate through all lights in SSBO (UNLIMITED!)
    for (int i = 0; i < u_NumLights; ++i) {
        LightData light = lights[i];

        if (light.type == LIGHT_TYPE_DIRECTIONAL) {
            // Directional light
            Lo += calculateDirectionalLight(light, albedo, normal, fs_in.FragPos, V, metallic, roughness, F0, directionalLightIndex);

            // Per-light ambient (no attenuation for directional)
            totalAmbient += light.color * light.ambientIntensity * albedo * ao;

            directionalLightIndex++;
        }
        else if (light.type == LIGHT_TYPE_POINT) {
            // Point light
            Lo += calculatePointLight(light, albedo, normal, fs_in.FragPos, V, metallic, roughness, F0, pointLightIndex);

            // Per-light ambient (attenuated by distance)
            float distance = length(light.position - fs_in.FragPos);
            float attenuation = 1.0 / (light.constant + light.linear * distance +
                                       light.quadratic * (distance * distance));
            totalAmbient += light.color * light.ambientIntensity * attenuation * albedo * ao;

            pointLightIndex++;
        }
        else if (light.type == LIGHT_TYPE_SPOT) {
            // Spot light
            Lo += calculateSpotLight(light, albedo, normal, fs_in.FragPos, V, metallic, roughness, F0, spotLightIndex);

            // Per-light ambient (attenuated by distance)
            float distance = length(light.position - fs_in.FragPos);
            float attenuation = 1.0 / (light.constant + light.linear * distance +
                                       light.quadratic * (distance * distance));
            totalAmbient += light.color * light.ambientIntensity * attenuation * albedo * ao;

            spotLightIndex++;
        }
    }

    // Add global ambient as fallback (can be set to 0 if using only per-light ambient)
    totalAmbient += u_AmbientLight * albedo * ao;

    return totalAmbient + Lo;
}

// ===== FOG FUNCTIONS (OGLDev Tutorial 39-style) =====

// Fog type constants (must match C++ FogType enum)
const int FOG_TYPE_NONE = 0;
const int FOG_TYPE_LINEAR = 1;
const int FOG_TYPE_EXPONENTIAL = 2;
const int FOG_TYPE_EXPONENTIAL_SQUARED = 3;

// Linear fog: interpolates linearly between start and end distances
float CalcLinearFogFactor()
{
    float cameraToPixelDist = length(fs_in.FragPos - u_ViewPos);
    float fogRange = u_FogEnd - u_FogStart;
    float fogDist = u_FogEnd - cameraToPixelDist;
    float fogFactor = fogDist / fogRange;
    return clamp(fogFactor, 0.0, 1.0); // 0.0 = full fog, 1.0 = no fog
}

// Exponential fog: density increases exponentially with distance
float CalcExpFogFactor()
{
    float cameraToPixelDist = length(fs_in.FragPos - u_ViewPos);
    float distRatio = 4.0 * cameraToPixelDist / u_FogEnd;
    float fogFactor = exp(-distRatio * u_FogDensity);
    return fogFactor;
}

// Exponential squared fog: smoother density falloff (most natural-looking)
float CalcExpSquaredFogFactor()
{
    float cameraToPixelDist = length(fs_in.FragPos - u_ViewPos);
    float distRatio = 4.0 * cameraToPixelDist / u_FogEnd;
    float fogFactor = exp(-distRatio * u_FogDensity * distRatio * u_FogDensity);
    return fogFactor;
}

// Calculate fog factor based on current fog type
float CalcFogFactor()
{
    if (u_FogType == FOG_TYPE_LINEAR) {
        return CalcLinearFogFactor();
    } else if (u_FogType == FOG_TYPE_EXPONENTIAL) {
        return CalcExpFogFactor();
    } else if (u_FogType == FOG_TYPE_EXPONENTIAL_SQUARED) {
        return CalcExpSquaredFogFactor();
    }
    return 1.0; // No fog
}

void main() {
    // Sample traditional textures
    vec4 albedo = vec4(fs_in.InstanceColor.rgb, 1.0);
    if (u_HasDiffuseMap) {
        albedo *= texture(u_DiffuseMap, fs_in.TexCoords);
    }

    float metallic = fs_in.InstanceMetallic;
    if (u_HasMetallicMap) {
        metallic *= texture(u_MetallicMap, fs_in.TexCoords).r;
    }

    float roughness = fs_in.InstanceRoughness;
    if (u_HasRoughnessMap) {
        // Modern PBR: Use roughness texture directly
        roughness *= texture(u_RoughnessMap, fs_in.TexCoords).r;
    }
    else if (u_HasSpecularMap) {
        // Legacy Blinn-Phong: Convert specular to roughness (backwards compatibility)
        // Specular maps are glossy (high = shiny), roughness is opposite (high = rough)
        float specular = texture(u_SpecularMap, fs_in.TexCoords).r;
        roughness = 1.0 - specular;  // Direct linear conversion (Option 1: more glossy)
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
    vec3 color = calculateMultiLightPBR(albedo.rgb, normal, metallic, roughness, ao);

    // Add emissive
    color += emissive;

    // Apply fog (OGLDev Tutorial 39-style)
    if (u_FogType != FOG_TYPE_NONE) {
        float fogFactor = CalcFogFactor(); // 0.0 = full fog, 1.0 = no fog
        color = mix(u_FogColor, color, fogFactor);
    }

    // Output raw HDR color to HDR framebuffer with appropriate alpha
    // Opaque pass: Force alpha = 1.0 to prevent texture alpha from affecting visibility
    // Transparent pass: Preserve texture alpha for proper blending
    // Tone mapping will be applied later in ToneMapRenderPass
    float outputAlpha = 1.0f;
    if (!u_IsOpaquePass) {
        outputAlpha = albedo.a;
    }
    FragColor = vec4(color, outputAlpha);
}