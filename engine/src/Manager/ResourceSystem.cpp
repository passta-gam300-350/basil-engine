#include "Manager/ResourceSystem.hpp"
#include "native/mesh.h"
#include "Render/render.h"
#include <yaml-cpp/yaml.h>

std::unique_ptr<ResourceSystem> ResourceSystem::m_Instance{};

namespace YAML {
    template<>
    struct convert<ResourceEntry> {
        static Node encode(const ResourceEntry& rhs) {
            Node node;
            node["guid"] = rhs.m_guid.to_hex();
            node["path"] = rhs.file_path;
            return node;
        }

        static bool decode(const Node& node, ResourceEntry& rhs) {
            if (!node.IsMap() || !node["guid"] || !node["path"])
                return false;
            rhs.m_guid = Resource::Guid::to_guid(node["guid"].as<std::string>());
            rhs.file_path = node["path"].as<std::string>();
            return true;
        }
    };
}

void ResourceSystem::Init(std::string const& asset_list) {
	if (!m_Instance) {
		m_Instance.reset(new ResourceSystem{});
	}
    if (asset_list.empty())
        return;
	ResourceSystem& rs{ GetInstance() };
	YAML::Node root{ YAML::Load(asset_list) };
	if (root["resources"] && root["resources"].IsSequence()) {
        std::vector<ResourceEntry> entries = root["resources"].as<std::vector<ResourceEntry>>();

        for (const auto& entry : entries) {
            rs.m_ResourceFileList[entry.m_guid] = entry.file_path;
        }
	}
}

ResourceSystem& ResourceSystem::GetInstance() {
	return *m_Instance;
}

void ResourceSystem::Load(Resource::Guid guid) {
    ResourceSystem& rs{ GetInstance() };
    assert(rs.m_ResourceFileList.find(guid) != rs.m_ResourceFileList.end());
    Load(rs.m_ResourceFileList[guid]);
}

void ResourceSystem::Load(std::string const& path) {
    ResourceSystem& rs{ GetInstance() };
    std::string ext{ path.substr(path.find_last_of(".") + 1) };
    if (ext == "mesh") {
        auto res = Resource::load_native_mesh(path);
        ResourceTypeMesh rtmesh{res};
        rs.m_Resources[res.m_guid] = std::make_shared<ResourceTypeMesh>(rtmesh);
    }
}
