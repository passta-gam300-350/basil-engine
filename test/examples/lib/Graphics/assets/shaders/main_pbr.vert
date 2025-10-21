#version 450 core

// Per-vertex attributes (from VAO)
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

// Instance data structure (must match C++ InstanceData exactly)
struct InstanceData {
    mat4 modelMatrix;      // 64 bytes
    vec4 color;            // 16 bytes
    uint materialId;       // 4 bytes
    uint flags;            // 4 bytes
    float metallic;        // 4 bytes
    float roughness;       // 4 bytes
    // Total: 96 bytes per instance
};

// Instance data SSBO
layout(std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};

// Camera uniforms
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform vec3 u_ViewPos;

// Shadow mapping uniforms
uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_SpotLightSpaceMatrix;

// Output to fragment shader
out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;

    // Shadow mapping
    vec4 FragPosLightSpace;
    vec4 FragPosSpotLightSpace;

    // Per-instance material data passed to fragment shader
    vec4 InstanceColor;
    float InstanceMetallic;
    float InstanceRoughness;
} vs_out;

void main()
{
    // Get instance data using gl_InstanceID for instanced rendering
    InstanceData instance = instances[gl_InstanceID];
    mat4 model = instance.modelMatrix;

    // Transform vertex to world space using instance matrix
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position = u_Projection * u_View * worldPos;

    // Pass data to fragment shader
    vs_out.FragPos = worldPos.xyz;
    vs_out.TexCoords = aTexCoords;

    // Calculate position in light space for shadow mapping
    vs_out.FragPosLightSpace = u_LightSpaceMatrix * worldPos;
    vs_out.FragPosSpotLightSpace = u_SpotLightSpaceMatrix * worldPos;

    // Calculate normal matrix for this instance
    mat3 normalMatrix = mat3(transpose(inverse(model)));

    // Transform normals and tangent space vectors
    vs_out.Normal = normalize(normalMatrix * aNormal);
    vs_out.Tangent = normalize(normalMatrix * aTangent);
    vs_out.Bitangent = normalize(normalMatrix * aBitangent);

    // Calculate TBN matrix for normal mapping
    vec3 T = normalize(vs_out.Tangent);
    vec3 B = normalize(vs_out.Bitangent);
    vec3 N = normalize(vs_out.Normal);
    vs_out.TBN = mat3(T, B, N);

    // Pass per-instance material data to fragment shader
    vs_out.InstanceColor = instance.color;
    vs_out.InstanceMetallic = instance.metallic;
    vs_out.InstanceRoughness = instance.roughness;
}