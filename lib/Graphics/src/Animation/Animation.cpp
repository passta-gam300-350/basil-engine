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
    }

}
boneChannel::boneChannel(std::string const& boneName, int id)
    : m_name(boneName), m_id(id), localTransform(1.0f)
{

}

void boneChannel::update(float time)
{

}



