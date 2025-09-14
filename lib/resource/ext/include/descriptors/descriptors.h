#ifndef LIB_RESOURCE_DESCRIPTORS_H
#define LIB_RESOURCE_DESCRIPTORS_H

#include <codecvt>
#include <locale>
#include <cstdint>
#include <string>
#include <map>
#include "serialisation/guid.h"
#include <fstream>

#include <yaml-cpp/yaml.h>

namespace Resource {

	enum class ResourceType : std::uint32_t {
		MESH,
		TEXTURE,
		MATERIAL,
		MODEL,
		VIDEO,
		AUDIO,
		UNKNOWN
	};

	struct ResourceDescriptor {
		Guid m_guid;
		ResourceType m_resource_type;
		std::wstring m_intermediate_files;
		std::string m_options;

		ResourceType deduce_resource_type(std::wstring_view wsv) {
			std::uint64_t pos{ wsv.find_last_of(L".") };
			ResourceType rtype{ ResourceType::UNKNOWN };
			if (pos != wsv.npos) {
				wsv = wsv.substr(pos + 1);
				if (wsv == L"obj" || wsv == L"fbx") {
					rtype = ResourceType::MODEL;
				}
				else if (wsv == L"png" || wsv == L"jpg") {
					rtype = ResourceType::TEXTURE;
				}
			}
			return rtype;
		}

		void generate_resource_descriptor_dependencies();

		ResourceDescriptor() = default;
		ResourceDescriptor(std::wstring file_name, std::string options = {}) 
			: m_guid{ Guid::generate() }, m_resource_type{ deduce_resource_type(file_name) }, m_intermediate_files{ file_name }, m_options{options}		
		{
			generate_resource_descriptor_dependencies();
		}

		void save_descriptor() {
			YAML::Node root;
			std::string str{ m_guid.to_hex_no_delimiter() };
			root["m_guid"] = str;
			root["m_resource_type"] = static_cast<std::uint32_t>(m_resource_type);
			str = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(m_intermediate_files);
			root["m_intermediate_files"] = str;
			root["m_options"] = m_options;

			str = str.substr(0, str.find_last_of(".")) + ".desc";
			std::ofstream outp{ str };
			outp << root;
		}
		static ResourceDescriptor load_descriptor(std::string_view sv) {
			ResourceDescriptor rdesc;
			YAML::Node root{ YAML::LoadFile(std::string(sv.begin(), sv.end())) };
			std::string str{ root["m_guid"].as<std::string>() };
			rdesc.m_guid = Guid::to_guid(str);
			rdesc.m_intermediate_files = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(root["m_intermediate_files"].as<std::string>());
			rdesc.m_resource_type = static_cast<ResourceType>(root["m_resource_type"].as<std::uint32_t>());
			rdesc.m_options = root["m_options"].as<std::string>();
			return rdesc;
		}
	};

	//tmp solution
	std::map<std::string, ResourceDescriptor>& get_virtual_descriptors();

}

#endif