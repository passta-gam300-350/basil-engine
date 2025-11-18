#version 450 core

in vec4 FragPos; // World-space position from geometry shader

uniform vec3 u_LightPos; // Point light position
uniform float u_FarPlane; // Far plane for normalization

void main()
{
    // Calculate distance between fragment and light source
    float lightDistance = length(FragPos.xyz - u_LightPos);

    // Map to [0, 1] range by dividing by far plane
    lightDistance = lightDistance / u_FarPlane;

    // Write distance as depth value
    gl_FragDepth = lightDistance;
}
