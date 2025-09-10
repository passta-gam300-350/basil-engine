#version 450 core

#extension GL_ARB_gpu_shader_int64 : enable

// Per-vertex attributes (from VAO)
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

// Instance transform matrices SSBO
layout(std430, binding = 0) buffer InstanceBuffer {
    mat4 modelMatrices[];
};

// Camera uniforms
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform vec3 u_ViewPos;

// Output to fragment shader
out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;
} vs_out;

void main()
{
    // Get transform matrix using gl_InstanceID for instanced rendering
    mat4 model = modelMatrices[gl_InstanceID];
    
    // Transform vertex to world space using instance matrix
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position = u_Projection * u_View * worldPos;
    
    // Pass data to fragment shader
    vs_out.FragPos = worldPos.xyz;
    vs_out.TexCoords = aTexCoords;
    
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
}