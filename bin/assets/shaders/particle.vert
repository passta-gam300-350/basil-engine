#version 450 core

layout(location = 0) in vec3 aPosition;

struct ParticleInstanceData
{
    vec3 position;
    float size;
    vec4 color;
    float rotation;
    uint textureID;
    uint padding[2];
};

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
    // Access the particle data for this instance
    vec3 particlePos = allParticles[gl_InstanceID].position;
    float particleSize = allParticles[gl_InstanceID].size;
    vec4 particleColor = allParticles[gl_InstanceID].color;
    float particleRotation = allParticles[gl_InstanceID].rotation;

    // Extract camera basis vectors from view matrix for billboarding
    vec3 cameraRight = vec3(u_View[0][0], u_View[1][0], u_View[2][0]);
    vec3 cameraUp = vec3(u_View[0][1], u_View[1][1], u_View[2][1]);

    // Apply rotation to the quad vertices
    float c = cos(particleRotation);
    float s = sin(particleRotation);
    vec2 rotatedPos = vec2(
        aPosition.x * c - aPosition.y * s,
        aPosition.x * s + aPosition.y * c
    );

    // Billboard the particle quad to face the camera
    vec3 vertexWorldPos = particlePos +
                          cameraRight * rotatedPos.x * particleSize +
                          cameraUp * rotatedPos.y * particleSize;

    // Final position in clip space
    gl_Position = u_Projection * u_View * vec4(vertexWorldPos, 1.0);

    // Pass data to fragment shader
    vTexCoord = aPosition.xy + 0.5;
    vColor = particleColor;
}