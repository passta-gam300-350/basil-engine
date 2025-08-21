#version 450 core

out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;
} fs_in;

// Material textures
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_height1;
uniform sampler2D texture_ambient1;
uniform sampler2D texture_emissive1;
uniform sampler2D texture_metallic1;
uniform sampler2D texture_roughness1;
uniform sampler2D texture_ao1;

// Material properties (fallbacks when textures are not available)
uniform vec3 u_AlbedoColor = vec3(0.8, 0.8, 0.8);
uniform float u_Metallic = 0.0;
uniform float u_Roughness = 0.5;
uniform float u_AO = 1.0;
uniform vec3 u_EmissiveColor = vec3(0.0);

// Light properties
uniform vec3 u_LightDir = vec3(0.5, -1.0, -0.3);
uniform vec3 u_LightColor = vec3(1.0, 1.0, 1.0);
uniform float u_LightIntensity = 1.0;
uniform vec3 u_AmbientColor = vec3(0.03);

// Camera
uniform vec3 u_ViewPos;

// Flags for available textures
uniform bool u_HasDiffuseMap = false;
uniform bool u_HasNormalMap = false;
uniform bool u_HasMetallicMap = false;
uniform bool u_HasRoughnessMap = false;
uniform bool u_HasAOMap = false;
uniform bool u_HasEmissiveMap = false;

vec3 getNormalFromMap()
{
    if (!u_HasNormalMap) {
        return normalize(fs_in.Normal);
    }
    
    vec3 tangentNormal = texture(texture_normal1, fs_in.TexCoords).xyz * 2.0 - 1.0;
    return normalize(fs_in.TBN * tangentNormal);
}

// Simple Blinn-Phong lighting (can be extended to PBR)
vec3 calculateLighting(vec3 albedo, vec3 normal, float metallic, float roughness, float ao)
{
    vec3 lightDir = normalize(-u_LightDir);
    vec3 viewDir = normalize(u_ViewPos - fs_in.FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    
    // Ambient
    vec3 ambient = u_AmbientColor * albedo * ao;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * u_LightColor * u_LightIntensity * albedo;
    
    // Specular (simplified)
    float spec = pow(max(dot(normal, halfwayDir), 0.0), mix(8.0, 128.0, 1.0 - roughness));
    vec3 specular = spec * u_LightColor * u_LightIntensity * mix(vec3(0.04), albedo, metallic);
    
    return ambient + diffuse + specular;
}

void main()
{
    // Get albedo
    vec3 albedo = u_HasDiffuseMap ? texture(texture_diffuse1, fs_in.TexCoords).rgb : u_AlbedoColor;
    
    // Get normal
    vec3 normal = getNormalFromMap();
    
    // Get metallic, roughness, AO
    float metallic = u_HasMetallicMap ? texture(texture_metallic1, fs_in.TexCoords).r : u_Metallic;
    float roughness = u_HasRoughnessMap ? texture(texture_roughness1, fs_in.TexCoords).r : u_Roughness;
    float ao = u_HasAOMap ? texture(texture_ao1, fs_in.TexCoords).r : u_AO;
    
    // Calculate lighting
    vec3 color = calculateLighting(albedo, normal, metallic, roughness, ao);
    
    // Add emissive
    if (u_HasEmissiveMap) {
        color += texture(texture_emissive1, fs_in.TexCoords).rgb;
    } else {
        color += u_EmissiveColor;
    }
    
    // Tone mapping and gamma correction
    color = color / (color + vec3(1.0)); // Reinhard tone mapping
    color = pow(color, vec3(1.0/2.2));   // Gamma correction
    
    FragColor = vec4(color, 1.0);
}