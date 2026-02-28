#version 450 core

layout(location = 0) in vec2 aPosition;   // 0-1 quad
layout(location = 1) in vec2 aTexCoord;

struct WorldUIInstanceData {
    vec3 worldPosition; float _pad0;
    vec3 billboardRight; float _pad1;
    vec3 billboardUp; float _pad2;
    vec2 size; vec2 _pad3;
    vec4 uvRect;   // min.xy, max.zw (for sprite sheets; default 0,0,1,1)
    vec4 color;
};

layout(std430, binding = 5) buffer WorldUIInstanceBuffer {
    WorldUIInstanceData instances[];
};

uniform mat4 u_View;
uniform mat4 u_Projection;

out vec2 vTexCoord;
out vec4 vColor;

void main() {
    WorldUIInstanceData inst = instances[gl_InstanceID];

    // Center the quad (-0.5 to 0.5)
    vec2 localPos = aPosition - vec2(0.5);

    // Build world position using billboard basis vectors
    vec3 worldPos = inst.worldPosition
        + inst.billboardRight * (localPos.x * inst.size.x)
        + inst.billboardUp * (localPos.y * inst.size.y);

    gl_Position = u_Projection * u_View * vec4(worldPos, 1.0);

    // Map texture coordinates through UV rect
    vec2 uvMin = inst.uvRect.xy;
    vec2 uvMax = inst.uvRect.zw;
    vTexCoord = mix(uvMin, uvMax, aTexCoord);

    vColor = inst.color;
}
