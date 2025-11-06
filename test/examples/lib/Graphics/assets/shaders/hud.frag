#version 450 core

// Inputs from vertex shader
in vec2 vTexCoord;
in vec4 vColor;

// Uniforms
uniform sampler2D u_Texture;
uniform bool u_UseTexture;

// Output
out vec4 FragColor;

void main()
{
    if (u_UseTexture)
    {
        // Sample texture and multiply by tint color
        vec4 texColor = texture(u_Texture, vTexCoord);
        FragColor = texColor * vColor;
    }
    else
    {
        // No texture, use solid color
        FragColor = vColor;
    }

    // Discard fully transparent pixels
    if (FragColor.a < 0.01)
    {
        discard;
    }
}
