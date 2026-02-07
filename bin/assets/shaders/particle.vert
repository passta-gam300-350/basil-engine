#version 450 core

layout(location = 0) in vec3 aPosition;

// Define struct OUTSIDE the buffer block
struct ParticleInstanceData
{
    vec3 position;
    float size;
    vec4 color;
    float rotation;
    uint textureID;
    uint padding[2];
};

// Then use it in the buffer
layout(std430, binding = 0) buffer ParticleBuffer
{
    ParticleInstanceData allParticles[];
};

uniform mat4 u_View;
uniform mat4 u_Projection;

out vec2 vTexCoord;
out vec4 vColor;

void main()
{
    ParticleInstanceData eachParticle = allParticles[gl_InstanceID];
    vec3 cameraRight = vec3(u_View[0][0], u_View[1][0], u_View[2][0]);
    vec3 cameraUp = vec3(u_View[0][1], u_View[1][1], u_View[2][1]);
    float c = cos(eachParticle.rotation);
    float s = sin(eachParticle.rotation);
    // rotation around billboard facing axis
    vec2 rotatedPos =
    vec2(
          aPosition.x * c - aPosition.y * s,
          aPosition.x * s + aPosition.y * c
        );
    vec3 vertexWorldPos = eachParticle.position + cameraRight * rotatedPos.x * eachParticle.size + cameraUp * rotatedPos.y * eachParticle.size;
    gl_Position = u_Projection * u_View * vec4(vertexWorldPos, 1.0);
    vTexCoord = aPosition.xy + 0.5;
    vColor = eachParticle.color;
}