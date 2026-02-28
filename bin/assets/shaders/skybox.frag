#version 460 core

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube u_Skybox;
uniform vec3 u_Tint;
uniform float u_Exposure;

void main() {
    vec3 color = texture(u_Skybox, TexCoords).rgb;
    FragColor = vec4(color * u_Tint * u_Exposure, 1.0);
}