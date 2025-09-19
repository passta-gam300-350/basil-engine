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

// Texture availability flags (set by texture slot system)
uniform bool u_HasDiffuseMap = false;
uniform bool u_HasNormalMap = false;
uniform bool u_HasMetallicMap = false;
uniform bool u_HasRoughnessMap = false;
uniform bool u_HasAOMap = false;
uniform bool u_HasEmissiveMap = false;
uniform bool u_HasSpecularMap = false;
uniform bool u_HasHeightMap = false;

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
vec3 calculateDirectionalLight(DirectionalLight light, vec3 albedo, vec3 normal, vec3 viewDir, float metallic, float roughness, vec3 F0, float shadowFactor)
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

    // Apply shadow to directional light contribution
    vec3 lightContribution = (kD * albedo / PI + specular) * radiance * NdotL;
    return lightContribution * (1.0 - shadowFactor * 0.8); // Apply 80% shadow intensity
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
vec3 calculateMultiLightPBR(vec3 albedo, vec3 normal, float metallic, float roughness, float ao, float shadowFactor)
{
    vec3 N = normalize(normal);
    vec3 V = normalize(u_ViewPos - fs_in.FragPos);

    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Accumulate lighting contribution
    vec3 Lo = vec3(0.0);

    // Point lights (no shadows for now)
    for (int i = 0; i < u_NumPointLights && i < 8; ++i) {
        Lo += calculatePointLight(u_PointLights[i], albedo, normal, fs_in.FragPos, V, metallic, roughness, F0);
    }

    // Directional lights (with shadows)
    for (int i = 0; i < u_NumDirectionalLights && i < 4; ++i) {
        Lo += calculateDirectionalLight(u_DirectionalLights[i], albedo, normal, V, metallic, roughness, F0, shadowFactor);
    }

    // Spot lights (no shadows for now)
    for (int i = 0; i < u_NumSpotLights && i < 4; ++i) {
        Lo += calculateSpotLight(u_SpotLights[i], albedo, normal, fs_in.FragPos, V, metallic, roughness, F0);
    }

    // Ambient lighting (not affected by shadows)
    vec3 ambient = vec3(0.03) * albedo * ao;

    return ambient + Lo;
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

    // Calculate shadow factor
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace);

    // DEBUG: Visualize shadow factor directly (comment out for normal rendering)
    // FragColor = vec4(vec3(1.0 - shadow), 1.0);
    // return;

    // Calculate multi-light PBR lighting with shadows
    vec3 color = calculateMultiLightPBR(albedo, normal, metallic, roughness, ao, shadow);

    // Add emissive
    color += emissive;

    // Tone mapping and gamma correction
    color = color / (color + vec3(1.0)); // Reinhard tone mapping
    color = pow(color, vec3(1.0/2.2));   // Gamma correction

    FragColor = vec4(color, 1.0);
}