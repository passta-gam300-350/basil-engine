#version 330 core

// Input vertex attributes
layout(location = 0) in vec3 a_Position;

// Uniforms for light-space transformation
uniform mat4 u_Model;
uniform mat4 u_View;        // Light view matrix
uniform mat4 u_Projection;  // Light projection matrix

void main()
{
    // Transform vertex to light-space for depth rendering
    gl_Position = u_Projection * u_View * u_Model * vec4(a_Position, 1.0);
}