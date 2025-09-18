#include "Manager/ResourceSystem.hpp"
#include "native/mesh.h"
#include "Render/render.h"
#include <yaml-cpp/yaml.h>

namespace YAML {
    template<>
    struct convert<ResourceSystem::FileEntry> {
        static Node encode(const ResourceSystem::FileEntry& rhs) {
            Node node;
            node["guid"] = rhs.m_Guid.to_hex();
            node["path"] = rhs.m_Path;
            node["byte"] = rhs.m_Offset; //byte offset
            node["size"] = rhs.m_Size;
            return node;
        }

        static bool decode(const Node& node, ResourceSystem::FileEntry& rhs) {
            if (!node.IsMap() || !node["guid"] || !node["path"])
                return false;
            rhs.m_Guid = Resource::Guid::to_guid(node["guid"].as<std::string>());
            rhs.m_Path = node["path"].as<std::string>();
            rhs.m_Path = node["byte"].as<std::uint64_t>();
            rhs.m_Size = node["size"].as<std::uint64_t>();
            return true;
        }
    };
}

const char* ResourceSystem::GetMappedFilePtr(Resource::Guid guid)
{
    assert(m_FileEntries.exist(guid) && "File entry does not exist!");
    FileEntry f_entry{ m_FileEntries[guid] };
    return reinterpret_cast<const char*>(m_MappedIO[f_entry.m_Path].getRange(f_entry.m_Offset, f_entry.m_Size));
}

void ResourceSystem::LoadFileLists(std::string_view filelist)
{
    ResourceSystem& rs{ Instance() };
    YAML::Node root{ YAML::Load(std::string(filelist.data(), filelist.length())) };
    if (root["resources"] && root["resources"].IsSequence()) {
        std::vector<ResourceSystem::FileEntry> entries = root["resources"].as<std::vector<ResourceSystem::FileEntry>>();

        for (const auto& entry : entries) {
            rs.m_FileEntries[entry.m_Guid] = entry;
        }
    }
}
