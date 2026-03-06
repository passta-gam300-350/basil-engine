/******************************************************************************/
/*!
\file   Animation.cpp
\author Team PASSTA
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Implementation of skeletal animation system with bone hierarchy and blending

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Animation/Animation.h"
#include <glm/gtx/compatibility.hpp>
#include <algorithm>
#include <iostream>
#include <set>

namespace helperTools
{
    // helper: find which "before" keyframe index we are at
    template<typename keyFrame>
    int findKeyFrameIndex(std::vector<keyFrame> const& allKeyFrames, float time)
    {
        for (int i = 0; i < allKeyFrames.size() - 1; i++)
        {
            if (time < allKeyFrames[i + 1].timeStamp)
            {
                return i; // index found, currently between i and i+1
            }
        }
        return static_cast<int>(allKeyFrames.size()) - 2; // between the second last and last already, return the second last
    }

    template <typename keyFrame>
    float clampT(std::vector<keyFrame> const& allKeyFrames, int keyFrameIndex, float currentTime)
    {
        float startTime = allKeyFrames[keyFrameIndex].timeStamp;
        float endTime = allKeyFrames[keyFrameIndex + 1].timeStamp;
        float totalDistanceInTime = endTime - startTime;

        if (totalDistanceInTime <= 0.0f)
        {
            assert(false && "Keyframe timestamps must be strictly increasing, data wrong!");
        }
        float t = (currentTime - startTime) / totalDistanceInTime;
        t = glm::clamp(t, 0.0f, 1.0f);
        return t;
    }

    void printSkeleton(const skeleton& theSkeleton)
    {
        for (size_t i = 0; i < theSkeleton.bones.size(); i++)
        {
            std::cout << "Bone " << i << ": " << theSkeleton.bones[i].name
                << " parent=" << theSkeleton.bones[i].parentIndex
                << std::endl;
        }
    }
}

namespace skeletonHelper
{
    bool validateHierarchy(const skeleton& theSkeleton)
    {
        // Check if parent indices are correct
        for (size_t i = 0; i < theSkeleton.bones.size(); i++)
        {
            if (theSkeleton.bones[i].parentIndex >= i && theSkeleton.bones[i].parentIndex != -1)
            {
                return false;
            }
        }
        return true;
    }

    void reorderHierarchy(skeleton& theSkeleton)
    {
        // Reorder bones to ensure parents before children
        // step 1: setup
        std::vector<oneSkeletonBone> orderedBones;
        std::vector<int> oldToNew(theSkeleton.bones.size(), -1); // -1 means havent sorted yet
        // step 2: add all root bones first (parent always come first to make sure no )
        for (size_t i = 0; i < theSkeleton.bones.size(); i++)
        {
            if (theSkeleton.bones[i].parentIndex == -1) 
            {
                oldToNew[i] = static_cast<int>(orderedBones.size());  // remember: old index i to new index (current size)
                oneSkeletonBone bone = theSkeleton.bones[i];
                bone.id = static_cast<int>(orderedBones.size());  // update bone ID to match new position
                // parentIndex stays -1 (it's root)
                orderedBones.push_back(bone);
            }
            // old index helps to track whats change to what
        }
        // step 3: keep adding children whose parents are already added
        bool addedAny = true;
        while (addedAny && orderedBones.size() < theSkeleton.bones.size())
        {
            addedAny = false; // reset flag
            for (size_t i = 0; i < theSkeleton.bones.size(); i++)
            {
                if (oldToNew[i] != -1)
                {
                    continue;
                }
                // check if the current bone parent is added or is root
                int oldBonesParentIndex = theSkeleton.bones[i].parentIndex;
                if (oldBonesParentIndex == -1 || oldToNew[oldBonesParentIndex] != -1) // is this bone parent already in ordered list
                {
                    oldToNew[i] = static_cast<int>(orderedBones.size());
                    oneSkeletonBone bone = theSkeleton.bones[i];
                    bone.id = static_cast<int>(orderedBones.size());  // update bone ID to match new position
                    if (oldBonesParentIndex != -1)
                    {
                        bone.parentIndex = oldToNew[oldBonesParentIndex];  // Parent's NEW position
                    }
                    else
                    {
                        bone.parentIndex = -1;  // Still root
                    }

                    orderedBones.push_back(bone);
                    addedAny = true;  // We added something, continue while loop
                }
            }
        }
        // step 4 verify if all processed
        if (orderedBones.size() != theSkeleton.bones.size())
        {
            return;  // do not modify skeleton if something went wrong
        }
        // all good, update the reordered to the original bones
        theSkeleton.bones = orderedBones; 
    }

    bool hasCircularDependency(const skeleton& theSkeleton)
    {
        // check for loops in hierarchy
        for (size_t i = 0; i < theSkeleton.bones.size(); i++)
        {
            std::set<int> visited;  // track where we've been
            int current = static_cast<int>(i);        // start from this bone

            // follow the parent chain
            while (current != -1)  // -1 means we reached the root
            {
                // have we seen this bone before?
                if (visited.count(current) > 0)
                {
                    return true;
                }
                // mark that we've been here
                visited.insert(current);
                // move to parent
                current = theSkeleton.bones[current].parentIndex;
            }
        }
        return false;  // No loops found
    }
}

namespace blendingHelper
{
    glm::mat4 blendTransforms(glm::mat4 const& firstT, glm::mat4 const& secondT, float blendFactor)
    {
        // extract translation 
        glm::vec3 firstPos = glm::vec3(firstT[3]);
        glm::vec3 secondPos = glm::vec3(secondT[3]);
        // extract rotation
        glm::quat firstRot = glm::quat_cast(firstT);
        glm::quat secondRot = glm::quat_cast(secondT);
        // extract scale
        glm::vec3 firstScale =
        glm::vec3(
            glm::length(glm::vec3(firstT[0])),
            glm::length(glm::vec3(firstT[1])),
            glm::length(glm::vec3(firstT[2]))
        );
        glm::vec3 secondScale = 
        glm::vec3(
            glm::length(glm::vec3(secondT[0])),
            glm::length(glm::vec3(secondT[1])),
            glm::length(glm::vec3(secondT[2]))
        );
        // blend all the components and reconstruct matrix
        glm::vec3 finalPosition = glm::mix(firstPos, secondPos, blendFactor);
        glm::quat finalRotation = glm::slerp(firstRot, secondRot, blendFactor);
        glm::vec3 finalScale = glm::mix(firstScale, secondScale, blendFactor);
        glm::mat4 matT = glm::translate(glm::mat4(1.0f), finalPosition);
        glm::mat4 matR = glm::mat4_cast(finalRotation);
        glm::mat4 matS = glm::scale(glm::mat4(1.0f), finalScale);
        return matT * matR * matS;
    }
}

boneChannel::boneChannel(std::string const& boneName, int id)
    : m_name(boneName), m_id(id), localTransform(1.0f)
{

}

void boneChannel::update(float time)
{
    assert(time >= 0.0f && "Animation time cannot be negative");
    glm::mat4 translation = interpolatePosition(time);
    glm::mat4 rotation = interpolateRotation(time);
    glm::mat4 scale = interpolateScale(time);
    // SRT
    localTransform = translation * rotation * scale;
}

glm::mat4 boneChannel::interpolatePosition(float time)
{
    if (m_positions.size() == 0) // edge cases, no position frame
    {
        return glm::mat4(1.0f);
    }
    if (m_positions.size() == 1)
    {
        return glm::translate(glm::mat4(1.0f), m_positions.front().position);
    }
    // get current frame
    int currentFrameIndex = helperTools::findKeyFrameIndex(m_positions, time);
    // get time (interpolation factor)
    float t = helperTools::clampT(m_positions, currentFrameIndex, time);
    // linear interpolate between two position 
    glm::vec3 finalPosition = glm::mix(m_positions[currentFrameIndex].position, m_positions[currentFrameIndex + 1].position, t);
    return glm::translate(glm::mat4(1.0f), finalPosition); //return the matrix
}

glm::mat4 boneChannel::interpolateRotation(float time)
{
    if (m_rotations.size() == 0)
    {
        return glm::mat4(1.0f); // no rotations
    }
    if (m_rotations.size() == 1)
    {
        return glm::mat4_cast(m_rotations.front().rotation);
    }
    // get current frame
    int currentFrameIndex = helperTools::findKeyFrameIndex(m_rotations, time);
    // get time (interpolation factor)
    float t = helperTools::clampT(m_rotations, currentFrameIndex, time);
    glm::quat finalRotation = glm::slerp(m_rotations[currentFrameIndex].rotation, m_rotations[currentFrameIndex + 1].rotation, t);
    return glm::mat4_cast(finalRotation);
}

glm::mat4 boneChannel::interpolateScale(float time)
{
    if (m_scales.size() == 0)
    {
        return glm::mat4(1.0f);
    }
    if (m_scales.size() == 1)
    {
        return glm::scale(glm::mat4(1.0f), m_scales.front().scale);
    }
    // get current frame
    int currentFrameIndex = helperTools::findKeyFrameIndex(m_scales, time);
    // get time (interpolation factor)
    float t = helperTools::clampT(m_scales, currentFrameIndex, time);
    glm::vec3 finalScale = glm::mix(m_scales[currentFrameIndex].scale, m_scales[currentFrameIndex + 1].scale, t);
    return glm::scale(glm::mat4(1.0f), finalScale);
}

void animator::updateAnimation(float deltaTime, skeleton const& theSkeleton)
{
    if (currentAnimation == nullptr)
    {
        return; // dont play animation
    }

    if (state.isPlaying == false)
    {
        return; // animation is paused or stop, no need update
    }

    assert(currentAnimation->duration > 0.0f && "Animation duration must be positive");
    assert(currentAnimation->ticksPerSecond > 0.0f && "Animation ticks per second must be positive");
    assert(deltaTime >= 0.0f && "Delta time cannot be negative");
    assert(state.playbackSpeed >= 0.0f && "Playback speed must be positive");

    if (blend.isActive == true)
    {
        updateBlendedAnimation(deltaTime, theSkeleton);
    }
    else
    {
        updateSingleAnimation(deltaTime, theSkeleton);
    }
   
}

void animator::play()
{
    state.isPlaying = true;
}

void animator::pause()
{
    state.isPlaying = false;
}

void animator::stop()
{
    state.isPlaying = false;
    currentTime = 0.0f;
}

void animator::setLoop(bool shouldLoop)
{
    state.loop = shouldLoop;
}

void animator::setPlayBackSpeed(float speed)
{
    state.playbackSpeed = speed;
}

bool animator::isAnimationFinished() const
{
    if (currentAnimation == nullptr)
    {
        return true; // no animation mean finished
    }
    return (!state.loop) && (currentTime >= currentAnimation->duration);
}

void animator::addAnimation(std::string const& animationName, animationContainer* animation)
{
    assert(animationName.empty() == false && "Animation name should not be empty");
    assert(animation != nullptr && "Null Animation should not be added");
    if (animation == nullptr)
    {
        return; // dont add useless animation
    }
    assert(animation->duration > 0.0f && "Animation duration must be positive");
    assert(animation->ticksPerSecond > 0.0f && "Animation ticks per second must be positive");
    assert(!animation->channels.empty() && "Animation must have at least one bone channel");
    allAnimations[animationName] = animation;
}

bool animator::playAnimation(std::string const& animationName, bool shouldLoop)
{
    assert(animationName.empty() == false && "Animation name should not be empty");
    auto currentPtr = allAnimations.find(animationName);
    if (currentPtr == allAnimations.end() || currentPtr->second == nullptr)
    {
        assert(false && ("Animation " + animationName + " not found or is null").c_str());
        return false;
    }
    animationContainer* newAnimation = currentPtr->second;
    // check for blend BEFORE changing currentAnimation
    if (currentAnimation != nullptr && state.isPlaying && currentAnimation != newAnimation)
    {
        blend.isActive = true;
        blend.sourceAnimation = currentAnimation;  // save the OLD one
        blend.sourceAnimationTime = currentTime;
    }
    // then change to new animation
    currentAnimation = newAnimation;
    currentTime = 0.0f;
    state.isPlaying = true;
    state.loop = shouldLoop;
    currentAnimationName = animationName;
    return true;
}

bool animator::hasAnimation(std::string const& animationName) const
{
    assert(animationName.empty() == false && "Animation name should not be empty");
    auto currentPtr = allAnimations.find(animationName);
    if (currentPtr == allAnimations.end() || currentPtr->second == nullptr)
    {
        return false;
    }
    return true;
}

std::string animator::getCurrentAnimationName() const
{
    return currentAnimationName;
}

void animator::updateSingleAnimation(float deltaTime, skeleton const& theSkeleton)
{
    // advance the time
    currentTime += currentAnimation->ticksPerSecond * deltaTime * state.playbackSpeed;
    // loop or not loop animation
    if (state.loop == true)
    {
        currentTime = fmod(currentTime, currentAnimation->duration);
    }
    else
    {
        if (currentTime >= currentAnimation->duration)
        {
            currentTime = currentAnimation->duration; // fix the time at the end of the animation
            state.isPlaying = false;
        }
    }
    // update all bone channels
    //create an array to store all local transformed bone
    std::vector<glm::mat4> localBoneTransforms(theSkeleton.bones.size(), glm::mat4(1.0f));  // store with identity matrix first
    for (boneChannel& eachChannel : currentAnimation->channels)
    {
        eachChannel.update(currentTime);
        int boneID = eachChannel.getID();
        if (boneID >= 0 && boneID < localBoneTransforms.size()) // need to ensure i got the correct bone ID
        {
            localBoneTransforms[boneID] = eachChannel.getLocalTransform();
        }
    }
    calculateFinalBoneMatrices(localBoneTransforms, theSkeleton);
}

void animator::updateBlendedAnimation(float deltaTime, skeleton const& theSkeleton)
{
    blend.currentTime += deltaTime;
    float blendFactor = blend.getBlendFactor();
    blend.sourceAnimationTime += blend.sourceAnimation->ticksPerSecond * deltaTime * state.playbackSpeed;
    if (state.loop == true)
    {
        blend.sourceAnimationTime = fmod(blend.sourceAnimationTime, blend.sourceAnimation->duration);
    }
    currentTime += currentAnimation->ticksPerSecond * deltaTime * state.playbackSpeed;
    if (state.loop == true)
    {
        currentTime = fmod(currentTime, currentAnimation->duration);
    }
    // Get source animation transforms
    std::vector<glm::mat4>
        sourceLocalTransforms(theSkeleton.bones.size(), glm::mat4(1.0f));
    for (boneChannel& channel : blend.sourceAnimation->channels)
    {
        channel.update(blend.sourceAnimationTime);
        int boneID = channel.getID();
        if (boneID >= 0 && boneID < sourceLocalTransforms.size())
        {
            sourceLocalTransforms[boneID] = channel.getLocalTransform();
        }
    }

    // Get target animation transforms
    std::vector<glm::mat4> targetLocalTransforms(theSkeleton.bones.size(), glm::mat4(1.0f));
    for (boneChannel& channel : currentAnimation->channels)
    {
        channel.update(currentTime);
        int boneID = channel.getID();
        if (boneID >= 0 && boneID < targetLocalTransforms.size())
        {
            targetLocalTransforms[boneID] = channel.getLocalTransform();
        }
    }

    // blend the transforms
    std::vector<glm::mat4> blendedLocalTransforms(theSkeleton.bones.size());
    for (size_t i = 0; i < theSkeleton.bones.size(); i++)
    {
        blendedLocalTransforms[i] = blendingHelper::blendTransforms(sourceLocalTransforms[i], targetLocalTransforms[i], blendFactor);
    }

    // Calculate final matrices
    calculateFinalBoneMatrices(blendedLocalTransforms, theSkeleton);

    // Check if blend complete
    if (blend.isComplete())
    {
        blend.isActive = false;
        blend.sourceAnimation = nullptr;
    }
}

void animator::calculateFinalBoneMatrices(std::vector<glm::mat4> const& localBoneTransforms, skeleton const& theSkeleton)
{
    // create an array to store global transform (apply the bone hierarchy)
    std::vector<glm::mat4> globalBoneTransforms(theSkeleton.bones.size(), glm::mat4(1.0f));
    for (size_t i = 0; i < theSkeleton.bones.size(); i++)
    {
        oneSkeletonBone const& eachBone = theSkeleton.bones[i];
        if (eachBone.parentIndex == -1)  // root bone (no parent)
        {
            globalBoneTransforms[i] = localBoneTransforms[i];
        }
        else
        {
            globalBoneTransforms[i] = globalBoneTransforms[eachBone.parentIndex] * localBoneTransforms[i];
        }
    }
    for (size_t i = 0; i < theSkeleton.bones.size(); i++)
    {
        finalBoneMatrices[i] = globalBoneTransforms[i] * theSkeleton.bones[i].inverseBind; // inverse bind is the bridge between the models original shape and the animated shape
        // original model space to animated model space
    }
}
