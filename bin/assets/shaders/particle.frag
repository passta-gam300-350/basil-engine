#version 450 core

in vec2 vTexCoord;
in vec4 vColor;

out vec4 FragColor;

void main()
{
	FragColor = vColor;
	if (FragColor.a < 0.01)
	{
		discard;
	}
}