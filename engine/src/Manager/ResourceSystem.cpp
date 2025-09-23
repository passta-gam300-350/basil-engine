#include "Manager/ResourceSystem.hpp"
#include "native/mesh.h"
#include "Render/render.h"
#include <yaml-cpp/yaml.h>
#include <filesystem>

namespace {
    constexpr std::uint64_t DEFAULT_RESOURCE_THREADS{ 4 };
    const std::string DEFAULT_RESOURCE_PATH{ std::filesystem::current_path().string() };
    constexpr bool DEFAULT_IS_GLOBBING_ENABLED{false};
}

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

void ResourceSystem::LoadConfig(YAML::Node& cfg) {
    if (auto res_thread_ct = cfg["resource threads"]; res_thread_ct) {
        SetResourceThreads(res_thread_ct.as<std::uint64_t>());
    }
    if (auto resource_list = cfg["resource list"]; resource_list) {
        LoadFileLists(resource_list.as<std::string>());
    }
    if (auto directory = cfg["root directory"]; directory) {
        Instance().m_ResourceRootDirectory = directory.as<std::string>();
    }
    if (auto glob_val = cfg["enable globbing"]; glob_val) {
        Instance().m_GlobFiles = glob_val.as<bool>();
    }
}

YAML::Node ResourceSystem::GetDefaultConfig(){
    YAML::Node root{};
    root["resource threads"] = DEFAULT_RESOURCE_THREADS;
    root["resource list"] = YAML::Node();
    root["root directory"] = DEFAULT_RESOURCE_PATH;
    root["enable globbing"] = DEFAULT_IS_GLOBBING_ENABLED;
    return root;
}