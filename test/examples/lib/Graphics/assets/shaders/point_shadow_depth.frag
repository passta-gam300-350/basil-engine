#version 400 core

in vec4 g_FragPos;  // World-space position from geometry shader

uniform vec3 u_LightPos;
uniform float u_FarPlane;

void main() {
    // Calculate distance from light to fragment
    float lightDistance = length(g_FragPos.xyz - u_LightPos);

    // Map to [0, 1] range
    lightDistance = lightDistance / u_FarPlane;

    // Write normalized distance as depth
    gl_FragDepth = lightDistance;
}