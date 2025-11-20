#version 450 core

// Vertex attributes for debug line rendering
layout (location = 0) in vec3 a_Position;  // World-space position
layout (location = 1) in vec3 a_Color;     // Per-vertex color from Jolt debug renderer

// Output to fragment shader
out VS_OUT {
    vec3 Color;
} vs_out;

// Camera transforms (no model matrix - debug lines are already in world space)
uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
    // Pass color through to fragment shader
    vs_out.Color = a_Color;

    // Transform position: debug lines are in world space, so no model matrix needed
    gl_Position = u_Projection * u_View * vec4(a_Position, 1.0);
}
