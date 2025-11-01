#version 450 core

out vec4 FragColor;

// Outline color uniform (can be changed for different outline colors)
uniform vec3 u_OutlineColor = vec3(1.0, 0.5, 0.0);  // Default: Orange

void main()
{
    FragColor = vec4(u_OutlineColor, 1.0);
}
