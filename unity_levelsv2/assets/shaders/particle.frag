#version 450 core

in vec2 vTexCoord; 
in vec4 vColor;

uniform sampler2D uTexture; // now everyone is same
out vec4 FragColor;

void main() 
{
	vec4 texColor = texture(uTexture, vTexCoord);
	FragColor = texColor * vColor;
	if (FragColor.a < 0.01)
	{
		discard;
	}
}