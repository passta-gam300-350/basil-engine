#version 460 core

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube u_Skybox;
uniform float u_Exposure = 1.0;           // HDR exposure multiplier
uniform vec3 u_Tint = vec3(1.0);          // Color tint
uniform mat4 u_RotationMatrix = mat4(1.0); // Rotation matrix

void main() {
    // Apply rotation to texture coordinates
    vec3 rotatedCoords = (u_RotationMatrix * vec4(TexCoords, 1.0)).xyz;

    // Sample cubemap with rotated coordinates
    vec3 color = texture(u_Skybox, rotatedCoords).rgb;

    // Apply tint
    color *= u_Tint;

    // Apply HDR exposure
    color *= u_Exposure;

    FragColor = vec4(color, 1.0);
}