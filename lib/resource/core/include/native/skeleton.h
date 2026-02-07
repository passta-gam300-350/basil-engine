#ifndef LIB_RESOURCE_CORE_NATIVE_SKELETON_H
#define LIB_RESOURCE_CORE_NATIVE_SKELETON_H

#include <vector>
#include <fstream>
#include <string>
#include "resource/utility.h"
#include <rsc-core/rp.hpp>
#include "serialization/native_serializer.h"
#include <glm/glm.hpp>


//serialised native format
class SkeletonResourceData {
public:
    struct Bone {
        int m_parent_index; //-1 is root
        std::string m_bone_name;
        int m_id;
        glm::vec4 m_inv_bind_c1; //i too lazy to update the reflection lib to work with private member pure interface accessors such as glm::mat4. this thing is gonna get copy constructed either ways
        glm::vec4 m_inv_bind_c2; //i too lazy to update the reflection lib to work with private member pure interface accessors such as glm::mat4. this thing is gonna get copy constructed either ways
        glm::vec4 m_inv_bind_c3; //i too lazy to update the reflection lib to work with private member pure interface accessors such as glm::mat4. this thing is gonna get copy constructed either ways
        glm::vec4 m_inv_bind_c4; //i too lazy to update the reflection lib to work with private member pure interface accessors such as glm::mat4. this thing is gonna get copy constructed either ways
        glm::vec4 local_c1; //i too lazy to update the reflection lib to work with private member pure interface accessors such as glm::mat4. this thing is gonna get copy constructed either ways
        glm::vec4 local_c2; //i too lazy to update the reflection lib to work with private member pure interface accessors such as glm::mat4. this thing is gonna get copy constructed either ways
        glm::vec4 local_c3; //i too lazy to update the reflection lib to work with private member pure interface accessors such as glm::mat4. this thing is gonna get copy constructed either ways
        glm::vec4 local_c4; //i too lazy to update the reflection lib to work with private member pure interface accessors such as glm::mat4. this thing is gonna get copy constructed either ways

    };
    std::vector<Bone> m_bones;
    std::string m_name;
};

#endif