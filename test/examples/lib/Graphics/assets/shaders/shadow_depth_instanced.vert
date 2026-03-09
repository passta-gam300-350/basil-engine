#version 450 core

// Per-vertex attributes (from VAO)
layout (location = 0) in vec3 aPos;

// Instance data structure (must match C++ InstanceData exactly)
struct InstanceData {
    mat4 modelMatrix;      // 64 bytes
    vec4 color;            // 16 bytes (not used in shadow pass)
    uint materialId;       // 4 bytes (not used in shadow pass)
    uint flags;            // 4 bytes (not used in shadow pass)
    float metallic;        // 4 bytes (not used in shadow pass)
    float roughness;       // 4 bytes (not used in shadow pass)
    float normalStrength;  // 4 bytes (not used in shadow pass)
    float padding;         // 4 bytes
    float padding2;        // 4 bytes (pad to multiple of 16)
    float padding3;        // 4 bytes
    // Total: 112 bytes per instance (16-byte aligned)
};

// Instance data SSBO
layout(std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};

// Light-space uniforms (same for all instances)
uniform mat4 u_View;        // Light view matrix
uniform mat4 u_Projection;  // Light projection matrix

void main()
{
    // Get instance data using gl_InstanceID
    InstanceData instance = instances[gl_InstanceID];
    mat4 model = instance.modelMatrix;

    // Transform vertex to light-space for depth rendering
    gl_Position = u_Projection * u_View * model * vec4(aPos, 1.0);
}
