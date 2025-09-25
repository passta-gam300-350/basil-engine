#ifndef LIB_RESOURCE_CORE_NATIVE_MESH_H
#define LIB_RESOURCE_CORE_NATIVE_MESH_H

#include <vector>
#include <fstream>
#include <string>
#include "serialisation/guid.h"
#include "utility.h"
#include <glm/glm.hpp>

#define MAX_BONE_INFLUENCE 4

namespace Resource {
    class MeshAssetData {
    public:
        struct Vertex {
            glm::vec3 Position;
            glm::vec3 Normal;
            glm::vec2 TexCoords;
            glm::vec3 Tangent;
            glm::vec3 Bitangent;
            int m_BoneIDs[MAX_BONE_INFLUENCE];
            float m_Weights[MAX_BONE_INFLUENCE];
        };

        // mesh Data
        std::vector<Vertex>       vertices;
        std::vector<unsigned int> indices;
        std::vector<Resource::Guid> textures;
        std::vector<std::byte> texture_type; //this is easier to serialise and texture types are < 255 so no issues here.

        static constexpr uint64_t MESH_MAGIC_VALUE{ iso8859ToBinary("E.MESH") };

        MeshAssetData& operator>>(std::ofstream&);
        MeshAssetData const& operator>>(std::ofstream&) const;

        //rets remaining buffer size
        std::uint64_t DumpToMemory(char* buff, std::uint64_t buffer_size) const;
    };
}

#endif