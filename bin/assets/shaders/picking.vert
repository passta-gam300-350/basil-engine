#version 460 core

layout (location = 0) in vec3 aPosition;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aWeights;

layout(std430, binding = 2) readonly buffer BoneMatrixBuffer {
    mat4 boneMatrices[];
};

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform bool u_EnableSkinning;
uniform int u_BoneOffset;

void main()
{
    vec3 pos = aPosition;

    if (u_EnableSkinning)
    {
        float totalWeight = aWeights.x + aWeights.y + aWeights.z + aWeights.w;
        if (totalWeight > 0.0)
        {
            mat4 skinMatrix =
                boneMatrices[u_BoneOffset + max(aBoneIDs.x, 0)] * aWeights.x +
                boneMatrices[u_BoneOffset + max(aBoneIDs.y, 0)] * aWeights.y +
                boneMatrices[u_BoneOffset + max(aBoneIDs.z, 0)] * aWeights.z +
                boneMatrices[u_BoneOffset + max(aBoneIDs.w, 0)] * aWeights.w;
            pos = vec3(skinMatrix * vec4(aPosition, 1.0));
        }
    }

    gl_Position = u_Projection * u_View * u_Model * vec4(pos, 1.0);
}
