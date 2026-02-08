/******************************************************************************/
/*!
\file   animation.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Native animation resource type

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef LIB_RESOURCE_CORE_NATIVE_ANIMATION_H
#define LIB_RESOURCE_CORE_NATIVE_ANIMATION_H

#include "resource/utility.h"
#include <rsc-core/rp.hpp>
#include "serialization/native_serializer.h"
#include <glm/glm.hpp>

//serialised native format
class AnimationResourceData {
public:
    struct Channel {
        std::string m_name;
        int m_id; 
        std::map<float, glm::vec3> m_positions;
        std::map<float, glm::quat> m_rotations;
        std::map<float, glm::vec3> m_scales;
    };
    std::vector<Channel> m_channels;
    std::string m_name;
    float m_ticks_per_sec;
    float m_duration;
};

#endif