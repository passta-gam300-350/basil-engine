#include "Animation/Animation.h"
#include <glm/gtx/compatibility.hpp>
#include <algorithm>

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
        return allKeyFrames.size() - 2; // between the second last and last already, return the second last
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
            return 0.0f;
        }
        float t = (currentTime - startTime) / totalDistanceInTime;
        t = glm::clamp(t, 0.0f, 1.0f);
        return t;
    }
}
boneChannel::boneChannel(std::string const& boneName, int id)
    : m_name(boneName), m_id(id), localTransform(1.0f)
{

}

void boneChannel::update(float time)
{
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



