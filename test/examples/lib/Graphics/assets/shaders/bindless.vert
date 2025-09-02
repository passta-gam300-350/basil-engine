#version 450 core

// Per-vertex attributes (from VAO)
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

// Simplified SSBO - just transform matrices (same as instanced.vert)
layout(std430, binding = 0) buffer InstanceBuffer {
    mat4 modelMatrices[];
};

// Camera uniforms (same as regular shader)
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform vec3 u_ViewPos;

// Output to fragment shader
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main()
{
    // Get transform matrix using gl_InstanceID
    mat4 model = modelMatrices[gl_InstanceID];
    
    // Transform vertex to world space using instance matrix
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position = u_Projection * u_View * worldPos;
    
    // Pass data to fragment shader
    FragPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(model))) * aNormal;  // Proper normal transformation
    TexCoords = aTexCoords;
}