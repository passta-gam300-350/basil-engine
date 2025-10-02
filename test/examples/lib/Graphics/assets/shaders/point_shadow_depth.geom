#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;  // 6 faces × 3 vertices

uniform mat4 u_ShadowMatrices[6];  // 6 shadow matrices (projection * view for each face)

in vec3 v_FragPos[];  // World-space position from vertex shader
out vec4 g_FragPos;   // Output to fragment shader

void main() {
    // Emit one triangle to each of the 6 cubemap faces
    for(int face = 0; face < 6; ++face) {
        gl_Layer = face;  // Built-in variable: selects cubemap face

        for(int i = 0; i < 3; ++i) {
            // Transform vertex to light space for this face
            g_FragPos = gl_in[i].gl_Position;  // World position
            gl_Position = u_ShadowMatrices[face] * g_FragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}