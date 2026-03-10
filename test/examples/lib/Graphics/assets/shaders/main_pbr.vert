#version 450 core

// Per-vertex attributes (from VAO)
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aWeights;

// Instance data structure (must match C++ InstanceData exactly)
struct InstanceData {
    mat4 modelMatrix;      // 64 bytes
    vec4 color;            // 16 bytes
    uint materialId;       // 4 bytes
    uint flags;            // 4 bytes
    float metallic;        // 4 bytes
    float roughness;       // 4 bytes
    float normalStrength;  // 4 bytes
    float padding;         // 4 bytes
    float padding2;        // 4 bytes (pad to multiple of 16)
    float padding3;        // 4 bytes
    // Total: 112 bytes per instance (16-byte aligned)
};

// Instance data SSBO
layout(std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};

layout(std430, binding = 2) readonly buffer BoneMatrixBuffer {
    mat4 boneMatrices[];
};

// Camera uniforms
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform vec3 u_ViewPos;
// Skinning control
uniform bool u_EnableSkinning;
uniform int u_BoneOffset;  // Offset into boneMatrices for this instance

// Output to fragment shader
out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;

    // Per-instance material data passed to fragment shader
    vec4 InstanceColor;
    float InstanceMetallic;
    float InstanceRoughness;
    float InstanceNormalStrength;
} vs_out;

void main()
{
    // Get instance data using gl_InstanceID for instanced rendering
    InstanceData instance = instances[gl_InstanceID];
    mat4 model = instance.modelMatrix;

    // === SKINNING ===
    vec3 skinnedPos = aPos;
    vec3 skinnedNormal = aNormal;
    vec3 skinnedTangent = aTangent;
    vec3 skinnedBitangent = aBitangent;
    if (u_EnableSkinning)
    {
        mat4 skinMatrix =
        boneMatrices[u_BoneOffset + aBoneIDs.x] * aWeights.x +
        boneMatrices[u_BoneOffset + aBoneIDs.y] * aWeights.y +
        boneMatrices[u_BoneOffset + aBoneIDs.z] * aWeights.z +
        boneMatrices[u_BoneOffset + aBoneIDs.w] * aWeights.w;

        skinnedPos = vec3(skinMatrix * vec4(aPos, 1.0));

        mat3 skinMatrix3 = mat3(skinMatrix);
        skinnedNormal = skinMatrix3 * aNormal;
        skinnedTangent = skinMatrix3 * aTangent;
        skinnedBitangent = skinMatrix3 * aBitangent;
    }
    // Transform vertex to world space using instance matrix
    vec4 worldPos = model * vec4(skinnedPos, 1.0);
    gl_Position = u_Projection * u_View * worldPos;

    // Pass data to fragment shader
    vs_out.FragPos = worldPos.xyz;
    vs_out.TexCoords = aTexCoords;

    // Calculate normal matrix for this instance
    mat3 normalMatrix = mat3(transpose(inverse(model)));

    // Transform normals and tangent space vectors
    vs_out.Normal = normalize(normalMatrix * skinnedNormal);
    vs_out.Tangent = normalize(normalMatrix * skinnedTangent);
    vs_out.Bitangent = normalize(normalMatrix * skinnedBitangent);

    // Calculate TBN matrix for normal mapping
    vec3 T = normalize(vs_out.Tangent);
    vec3 B = normalize(vs_out.Bitangent);
    vec3 N = normalize(vs_out.Normal);
    vs_out.TBN = mat3(T, B, N);

    // Pass per-instance material data to fragment shader
    vs_out.InstanceColor = instance.color;
    vs_out.InstanceMetallic = instance.metallic;
    vs_out.InstanceRoughness = instance.roughness;
    vs_out.InstanceNormalStrength = instance.normalStrength;
}