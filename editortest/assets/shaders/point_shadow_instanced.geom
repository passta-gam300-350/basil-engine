#version 450 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

// Light space matrices for all 6 cubemap faces
uniform mat4 u_ShadowMatrices[6];

// Output to fragment shader
out vec4 FragPos; // World-space position for distance calculation

void main()
{
    // Emit geometry to all 6 cubemap faces
    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = face; // Built-in variable that specifies cubemap face

        // Emit triangle for this face
        for(int i = 0; i < 3; ++i)
        {
            FragPos = gl_in[i].gl_Position; // World position
            gl_Position = u_ShadowMatrices[face] * FragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}
