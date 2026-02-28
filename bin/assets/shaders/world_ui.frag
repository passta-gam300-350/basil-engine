#version 450 core

in vec2 vTexCoord;
in vec4 vColor;

uniform sampler2D u_Texture;
uniform bool u_HasTexture;

out vec4 FragColor;

void main() {
    vec4 texColor = u_HasTexture ? texture(u_Texture, vTexCoord) : vec4(1.0);
    FragColor = texColor * vColor;
    if (FragColor.a < 0.01) discard;
}
