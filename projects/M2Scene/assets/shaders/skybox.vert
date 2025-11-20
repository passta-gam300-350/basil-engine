#version 460 core

layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 u_Projection;
uniform mat4 u_View;

void main() {
    TexCoords = aPos;

    // Remove translation from view matrix for skybox effect
    mat4 rotView = mat4(mat3(u_View));
    vec4 pos = u_Projection * rotView * vec4(aPos, 1.0);

    // Ensure skybox is always at maximum depth
    gl_Position = pos.xyww;
}