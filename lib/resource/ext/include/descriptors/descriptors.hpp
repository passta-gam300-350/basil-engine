#ifndef LIB_RESOURCE_DESCRIPTORS_H
#define LIB_RESOURCE_DESCRIPTORS_H

#include <codecvt>
#include <locale>
#include <cstdint>
#include <string>
#include "serialisation/guid.h"
#include <fstream>
#include <unordered_map>
#include "descriptors/descriptor_registry.hpp"

#include <yaml-cpp/yaml.h>

namespace YAML {
	template<>
	struct convert<std::unordered_map<std::string, std::string>> {
		static Node encode(const std::unordered_map<std::string, std::string>& rhs) {
			Node node;
			for (auto& [k, v] : rhs) {
				node[k] = v;
			}
			return node;
		}

		static bool decode(const Node& node, std::unordered_map<std::string, std::string>& rhs) {
			if (!node.IsMap()) return false;
			rhs.clear();
			for (auto it = node.begin(); it != node.end(); ++it) {
				rhs[it->first.as<std::string>()] = it->second.as<std::string>();
			}
			return true;
		}
	};
}


namespace Resource {

	enum class ResourceType : std::uint32_t {
		MESH,
		TEXTURE,
		MATERIAL,
		VIDEO,
		AUDIO,
		UNKNOWN
	};

	std::vector<std::string>& GetTypeVector();
	
	ResourceType GetResourceTypeFromName(std::string const& str);

	std::string GetResourceTypeName(ResourceType t);

	struct ResourceDescriptor {
		//struct ResourceDescriptorReference {
		//	std::string m_Ref; //reference descriptor path
		//};
		using ResourceDescriptorReference = std::string;
		struct ResourceDescriptorEntry { //individual resource in the file
			Guid m_Guid;
			std::string m_Name;
			std::unordered_map<std::string, ResourceDescriptorReference> m_References;
			std::string m_ImporterOptions;
		};
		struct ResourceMetadataInfo {
			std::uint64_t m_DateIndexed_ms{}; //date since unix epoch in ms
			std::uint64_t m_FileChecksumHash{}; //checksum hash for version control and to flag for dirty descriptors 
			std::string m_RawSourcePath;
		};

		ResourceMetadataInfo m_RawFileInfo; //info of the intermediate files
		std::unordered_multimap<ResourceType, ResourceDescriptorEntry> m_DescriptorEntries; //resource entries in the file. some file like fbx have multiple resource in one file

		void SaveDescriptor() {
			YAML::Node root{};
			YAML::Node file_info{};
			YAML::Node resource{};
			for (auto& [key, value] : m_DescriptorEntries) {
				YAML::Node entry{};
				entry["guid"] = value.m_Guid.to_hex_no_delimiter();
				entry["name"] = value.m_Name;
				entry["references"] = value.m_References;
				entry["options"] = value.m_ImporterOptions;
				resource[GetResourceTypeName(key)].push_back(entry);
			}
			file_info["date indexed"] = m_RawFileInfo.m_DateIndexed_ms;
			file_info["file hash"] = m_RawFileInfo.m_FileChecksumHash;
			file_info["file path"] = m_RawFileInfo.m_RawSourcePath;
			root["file info"] = file_info;
			root["resources"] = resource;
			std::string s{ Resource::DescriptorRegistry::GetDescriptorRootDirectory() + m_RawFileInfo.m_RawSourcePath.substr(0, m_RawFileInfo.m_RawSourcePath.find_last_of(".")) + ".desc"};
			std::ofstream ofs{ s };
			ofs << root;
		}

		static ResourceDescriptor LoadDescriptor(std::string const& s) {
			YAML::Node root{ YAML::LoadFile(s) };
			YAML::Node file_info{root["file info"]};
			YAML::Node resource{root["resources"]};
			ResourceDescriptor rdesc{};
			for (auto it = resource.begin(); it != resource.end(); ++it) {
				std::string key = it->first.as<std::string>();
				for (auto const& val : it->second) {
					ResourceDescriptorEntry entry{};
					entry.m_Guid = Resource::Guid::to_guid(val["guid"].as<std::string>());
					entry.m_ImporterOptions = val["options"].as<std::string>();
					if (val["reference"]) {
						entry.m_References = val["reference"].as<std::unordered_map<std::string, std::string>>();
					}
					entry.m_Name = val["name"].as<std::string>();
					rdesc.m_DescriptorEntries.emplace(GetResourceTypeFromName(key), entry);
				}
			}
			rdesc.m_RawFileInfo.m_DateIndexed_ms = file_info["date indexed"].as<std::uint64_t>();
			rdesc.m_RawFileInfo.m_FileChecksumHash = file_info["file hash"].as<std::uint64_t>();
			rdesc.m_RawFileInfo.m_RawSourcePath = file_info["file path"].as<std::string>();
			return rdesc;
		}
		static ResourceDescriptor LoadDescriptor(std::string_view sv) {
			return LoadDescriptor(std::string(sv.begin(), sv.end()));
		}

		ResourceDescriptor() = default;
		ResourceDescriptor(ResourceDescriptor const&) = default;
		ResourceDescriptor(std::string const& path) {
			new (this) ResourceDescriptor{ LoadDescriptor(path) };
		}
		~ResourceDescriptor() = default;

		static ResourceDescriptor MakeDescriptor(std::string const& s);
		static ResourceDescriptor MakeDescriptorTexture(std::string const& s);
		static ResourceDescriptor MakeDescriptorModel(std::string const& s);
	};

}

#endif