#version 450 core

// Output
out vec4 FragColor;

// Input from vertex shader
in VS_OUT {
    vec3 Color;
} fs_in;

void main()
{
    // Use per-vertex color from Jolt debug renderer
    // This preserves Jolt's color coding:
    // - Green: collision shapes
    // - Red: contact points/normals
    // - Blue/Cyan: constraints, velocities, etc.
    FragColor = vec4(fs_in.Color, 1.0);
}
