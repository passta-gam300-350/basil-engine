#version 450 core

// Input vertex attributes
layout(location = 0) in vec3 a_Position;

// Instance data structure (must match CPU-side InstanceData)
struct InstanceData {
    mat4 modelMatrix;      // 64 bytes
    vec4 color;            // 16 bytes (not used in shadow pass)
    uint materialId;       // 4 bytes (not used)
    uint flags;            // 4 bytes (not used)
    float metallic;        // 4 bytes (not used)
    float roughness;       // 4 bytes (not used)
    float normalStrength;  // 4 bytes (not used)
    float padding;         // 4 bytes
    float padding2;        // 4 bytes (pad to multiple of 16)
    float padding3;        // 4 bytes
    // Total: 112 bytes per instance (16-byte aligned)
};

// SSBO containing all instance data
layout(std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};

void main()
{
    // Fetch instance data using gl_InstanceID
    InstanceData instance = instances[gl_InstanceID];
    mat4 u_Model = instance.modelMatrix;

    // Pass world-space position to geometry shader
    // Geometry shader will apply view-projection for each cubemap face
    gl_Position = u_Model * vec4(a_Position, 1.0);
}
