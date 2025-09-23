#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
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
    glm::mat4 inverseBind; // used for skinning, moves vertex from model space into bones space
    // for eg We first move the vertex into elbow’s bind space (inverse bind) ? it becomes “1 unit away”.
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

struct animator
{
    std::unique_ptr<animationContainer> currentAnimation;
    float currentTime = 0.0f;
    // final matrices (one per bone, sent to GPU)
    std::vector<glm::mat4> finalBoneMatrices;
    animator(std::unique_ptr<animationContainer> theAnimation, int numberOfBones) : currentAnimation(std::move(theAnimation))
    {
        finalBoneMatrices.resize(numberOfBones);
    }
    void updateAnimation(float deltaTime, const skeleton& theSkeleton);
};

