#version 330 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_Model;

out vec3 v_FragPos;  // World-space position (pass to geometry shader)

void main() {
    // Transform to world space
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_FragPos = worldPos.xyz;

    // No view/projection here - geometry shader handles that for each face
    gl_Position = worldPos;
}