#ifndef LIB_RESOURCE_CORE_NATIVE_RIG_H
#define LIB_RESOURCE_CORE_NATIVE_RIG_H

#include <vector>
#include <fstream>
#include <string>
#include "resource/utility.h"
#include <rsc-core/rp.hpp>
#include "serialization/native_serializer.h"
#include <glm/glm.hpp>

#define MAX_BONE_INFLUENCE 4

//serialised native format
class RigResourceData {
public:
    struct Bone {
        std::string m_bone_name;
        int m_parent_index{ -1 }; //-1 is root
        glm::mat4 m_offset;
        glm::mat4 m_local_mtx;
    };
    std::vector<Bone> m_bones;
    std::unordered_map<std::string, int> m_bone_name_index_map;
    glm::mat4 m_global_inverse_mtx;
};

#endif