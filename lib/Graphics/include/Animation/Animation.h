/******************************************************************************/
/*!
\file   Animation.h
\author Team PASSTA
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares skeletal animation structures and animator for keyframe-based bone animation

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <unordered_map>

// key frame for position, will need for interpolation
struct keyFramePosition
{
    float timeStamp;
    glm::vec3 position;
    keyFramePosition(float t, glm::vec3 const& p) : timeStamp(t), position(p)
    {

    }
};

// key frame for rotation, will need for interpolation
struct keyFrameRotation
{
    float timeStamp;
    glm::quat rotation;
    keyFrameRotation(float t, glm::quat const& r) : timeStamp(t), rotation(r)
    {

    }
};

// key frame for scaleA, will need for interpolation
struct keyFrameScale
{
    float timeStamp;
    glm::vec3 scale;
    keyFrameScale(float t, glm::vec3 const& s) : timeStamp(t), scale(s)
    {

    }
};

// assimp node data (check with steven is needed, did he parse for me or i parse myself)
struct AssimpNodeData
{
    std::string name;
    glm::mat4 transformation;
    int childrenCount;
    std::vector<AssimpNodeData> children;
};

// one bone (static)
struct oneSkeletonBone
{
    std::string name; // bone name like arm, leg, shoulder
    int id; // index for GPU
    int parentIndex; // who is the bone parent, if it is root then -1
    glm::mat4 inverseBind; // used for skinning, moves vertex from model space into bones space (is like create a local coordinate system for each bone)
    // for eg We first move the vertex into elbow's bind space (inverse bind) ? it becomes "1 unit away".
};

// whole skeleton (static)
struct skeleton
{
    std::vector<oneSkeletonBone> bones;
};

// bone channel, each bone hold a channel
// how one bone moves over time
struct boneChannel
{
public:
    boneChannel(std::string const& boneName, int id);
    const std::string getName() const
    {
        return m_name;
    }
    int getID() const
    {
        return m_id;
    }
    glm::mat4 getLocalTransform() const
    {
        return localTransform;
    }
    void update(float time); // interpolate all the key frames
    // setter helper function
    void addPositionKeyframe(float time, glm::vec3 const& position)
    {
        m_positions.push_back(keyFramePosition(time, position));
    }

    void addRotationKeyframe(float time, glm::quat const& rotation)
    {
        m_rotations.push_back(keyFrameRotation(time, rotation));
    }

    void addScaleKeyframe(float time, glm::vec3 const& scale)
    {
        m_scales.push_back(keyFrameScale(time, scale));
    }

    // Optional: Clear all keyframes
    void clearKeyframes()
    {
        m_positions.clear();
        m_rotations.clear();
        m_scales.clear();
    }

private:
    std::string m_name;
    int m_id;
    std::vector<keyFramePosition> m_positions;
    std::vector<keyFrameRotation> m_rotations;
    std::vector<keyFrameScale> m_scales;
    glm::mat4 localTransform { 1.0f };
    // helper function for update to interpolate 
    glm::mat4 interpolatePosition(float time);
    glm::mat4 interpolateRotation(float time);
    glm::mat4 interpolateScale(float time);
};

// contains all those channels for one clips
struct animationContainer
{ 
    float duration; // total length in ticks
    float ticksPerSecond; // how fast animation time runs (some files use 24fps, 30fps, or custom).
    std::vector<boneChannel> channels; // one channel per bone
};

struct animationState
{
    bool isPlaying = true;
    bool loop = true;
    float playbackSpeed = 1.0f;
    float startTime = 0.0f;
    float endTime = -1.0f; // use full duration
};

struct blendState
{
    bool isActive = false;
    float currentTime = 0.0f;
    float duration = 0.3f;
    animationContainer* sourceAnimation = nullptr;
    float sourceAnimationTime = 0.0f;
    float getBlendFactor() const
    {
        if(isActive == false || duration <= 0.0f)
        {
            return 1.0f;
        }
        float t = glm::clamp(currentTime / duration, 0.0f, 1.0f);
        // smoothstep for nice ease-in-out
        return t * t * (3.0f - 2.0f * t);
    }
    bool isComplete() const
    {
        return currentTime >= duration;
    }
};

struct animator
{
    animationContainer* currentAnimation = nullptr; // current playing animation
    float currentTime = 0.0f;
    // final matrices (one per bone, sent to GPU)
    std::vector<glm::mat4> finalBoneMatrices;
    // the animation state
    animationState state;
    // all the animations need
    std::unordered_map<std::string, animationContainer*> allAnimations;
    // current animation name
    std::string currentAnimationName; 
    blendState blend;
    
    animator(int numberOfBones)
    {
        finalBoneMatrices.resize(numberOfBones);
    }
    void updateAnimation(float deltaTime, skeleton const& theSkeleton);
    void play();
    void pause();
    void stop();
    void setLoop(bool shouldLoop);
    void setPlayBackSpeed(float speed);
    bool isAnimationFinished() const;
    void addAnimation(std::string const& animationName, animationContainer* animation);
    bool playAnimation(std::string const& animationName, bool shouldLoop = true);
    bool hasAnimation(std::string const& animationName) const;
    std::string getCurrentAnimationName() const;
private:
    void updateSingleAnimation(float deltaTime, skeleton const& theSkeleton);
    void updateBlendedAnimation(float deltaTime, skeleton const& theSkeleton);
    void calculateFinalBoneMatrices(std::vector<glm::mat4> const& localBoneTransforms, skeleton const& theSkeleton);
};