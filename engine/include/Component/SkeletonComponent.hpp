/******************************************************************************/
/*!
\file   SkeletonComponent.hpp
\author Team PASSTA
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2026/01/23
\brief  Declares Skeleton Component for skeletal animation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef SKELETON_COMPONENT_HPP
#define SKELETON_COMPONENT_HPP

#include <glm/glm.hpp>
#include <vector>
#include "Animation/Animation.h"
#include <native/skeleton.h>
#include <rsc-core/rp.hpp>

// Maximum bones supported per skeleton (matches shader limit)
constexpr int MAX_BONES = 128;

RegisterResourceTypeForward(SkeletonResourceData, "skeleton", unusedskeletonstuff)

struct SkeletonComponent
{
    // The skeleton data (bone hierarchy, inverse bind matrices)
    skeleton skeletonData;
    rp::BasicIndexedGuid skeletondata{ static_cast<rp::BasicIndexedGuid>(rp::TypeNameGuid<"skeleton">()) };

    // Final bone matrices for GPU skinning (updated each frame by AnimationSystem)
    // These are: globalTransform * inverseBind for each bone
    std::vector<glm::mat4> finalBoneMatrices;

    // Global transforms (world-space bone poses, useful for attachments/debug)
    std::vector<glm::mat4> globalBoneTransforms;

    // Initialize matrices based on bone count
    void InitializeMatrices()
    {
        const size_t boneCount = skeletonData.bones.size();
        finalBoneMatrices.resize(boneCount, glm::mat4(1.0f));
        globalBoneTransforms.resize(boneCount, glm::mat4(1.0f));
    }

    // Get bone count
    size_t GetBoneCount() const
    {
        return skeletonData.bones.size();
    }

    // Find bone index by name (-1 if not found)
    int FindBoneIndex(const std::string& boneName) const
    {
        for (size_t i = 0; i < skeletonData.bones.size(); ++i)
        {
            if (skeletonData.bones[i].name == boneName)
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
};

// Note: SkeletonComponent contains runtime data (matrices) that shouldn't be serialized
// Only the skeleton reference/GUID would be serialized in a full implementation
// For now, skeleton data is set up programmatically or loaded with the model

RegisterReflectionTypeBegin(SkeletonComponent, "SkeletonComponent")
MemberRegistrationV<&SkeletonComponent::skeletondata, "SkeletonData">
// Note: channel is NOT registered (runtime pointer, not serializable)
RegisterReflectionTypeEnd

#endif
