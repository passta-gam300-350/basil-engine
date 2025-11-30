#ifndef LIB_RESOURCE_CORE_NATIVE_ANIMATION_H
#define LIB_RESOURCE_CORE_NATIVE_ANIMATION_H

#include "resource/utility.h"
#include <rsc-core/rp.hpp>
#include "serialization/native_serializer.h"
#include <glm/glm.hpp>

//serialised native format
class AnimationResourceData {
public:
    std::string m_name;
    int m_id;
    std::map<float, glm::vec3> m_positions;
    std::map<float, glm::quat> m_rotations;
    std::map<float, glm::vec3> m_scales;
};

#endif