#version 450 core

out vec4 FragColor;

in VS_OUT {
    vec2 TexCoords;
} fs_in;

// Simple color uniform for primitive rendering
uniform vec3 u_Color;

// Optional texture support
uniform sampler2D u_Texture;
uniform bool u_UseTexture = false;

void main()
{
    vec3 color = u_Color;

    if (u_UseTexture) {
        vec3 texColor = texture(u_Texture, fs_in.TexCoords).rgb;
        color = color * texColor;
    }

    FragColor = vec4(color, 1.0);
}