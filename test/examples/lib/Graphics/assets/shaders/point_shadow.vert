#version 450 core

// Input vertex attributes
layout(location = 0) in vec3 a_Position;

// Uniforms
uniform mat4 u_Model;

void main()
{
    // Pass position to geometry shader (no transformation yet)
    // Geometry shader will apply view-projection for each cubemap face
    gl_Position = u_Model * vec4(a_Position, 1.0);
}
