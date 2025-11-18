#version 460 core

out vec4 FragColor;

uniform float u_ObjectID;

void main()
{
    // Convert object ID to RGB color
    // u_ObjectID is normalized [0,1], we need to convert back to 24-bit RGB
    uint objectID = uint(u_ObjectID * 16777215.0); // 2^24 - 1

    float r = float((objectID >> 16u) & 0xFFu) / 255.0;
    float g = float((objectID >> 8u) & 0xFFu) / 255.0;
    float b = float(objectID & 0xFFu) / 255.0;

    FragColor = vec4(r, g, b, 1.0);
}