/******************************************************************************/
/*!
\file   mesh.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Native mesh resource type

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef LIB_RESOURCE_CORE_NATIVE_MESH_H
#define LIB_RESOURCE_CORE_NATIVE_MESH_H

#include <vector>
#include <fstream>
#include <string>
#include "resource/utility.h"
#include <rsc-core/rp.hpp>
#include "serialization/native_serializer.h"
#include <glm/glm.hpp>

#define MAX_BONE_INFLUENCE 4

//serialised native format
class MeshResourceData {
public:
    //per vertex data
    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
        glm::vec3 Tangent;
        glm::vec3 Bitangent;
        int m_BoneIDs[MAX_BONE_INFLUENCE];
        float m_Weights[MAX_BONE_INFLUENCE];
    };
    //material slots
    struct MaterialSlot {
        rp::Guid material_guid{rp::null_guid};
        std::string material_slot_name{"unnamed slot"};
        unsigned int index_begin; //submesh
        unsigned int index_count;
    };
    //mesh Data //lod ready (each mesh represents 1 lod)
    struct Mesh {
        std::vector<Vertex>       vertices;
        std::vector<unsigned int> indices;
        std::vector<MaterialSlot> materials;
    };
    std::vector<Mesh> meshes;
};

#endif