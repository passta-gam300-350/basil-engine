#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoords;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec3 Tangent;
    vec3 Bitangent;
    mat3 TBN;
} vs_out;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat3 u_NormalMatrix;

void main()
{
    vs_out.FragPos = vec3(u_Model * vec4(a_Position, 1.0));
    vs_out.TexCoords = a_TexCoords;
    
    // Calculate normal matrix if not provided
    mat3 normalMatrix = u_NormalMatrix;
    if (normalMatrix == mat3(0.0)) {
        normalMatrix = mat3(transpose(inverse(u_Model)));
    }
    
    vs_out.Normal = normalize(normalMatrix * a_Normal);
    vs_out.Tangent = normalize(normalMatrix * a_Tangent);
    vs_out.Bitangent = normalize(normalMatrix * a_Bitangent);
    
    // Calculate TBN matrix for normal mapping
    vec3 T = normalize(vs_out.Tangent);
    vec3 B = normalize(vs_out.Bitangent);
    vec3 N = normalize(vs_out.Normal);
    vs_out.TBN = mat3(T, B, N);
    
    gl_Position = u_Projection * u_View * vec4(vs_out.FragPos, 1.0);
}