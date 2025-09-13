#ifndef LIB_RESOURCE_DESCRIPTORS_H
#define LIB_RESOURCE_DESCRIPTORS_H

#include <cstdint>
#include <string>
#include <map>
#include "serialisation/guid.h"

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

		ResourceDescriptor() = default;
		ResourceDescriptor(std::wstring file_name, std::string options = {}) 
			: m_guid{ Guid::generate() }, m_resource_type{ deduce_resource_type(file_name) }, m_intermediate_files{ file_name }, m_options{options}		
		{}
	};

	//tmp solution
	std::map<std::string, ResourceDescriptor>& get_virtual_descriptors();

}

#endif