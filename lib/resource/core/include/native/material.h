#ifndef LIB_RESOURCE_CORE_NATIVE_MATERIAL_H
#define LIB_RESOURCE_CORE_NATIVE_MATERIAL_H

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include "serialisation/guid.h"

namespace Resource {
	struct MaterialAssetData {
		Guid shader_guid;
		std::string m_Name;

		// PBR Material Properties (previously in PBRMaterialProperties)
		glm::vec3 m_AlbedoColor = glm::vec3(0.8f, 0.7f, 0.6f);
		float m_MetallicValue = 0.7f;
		float m_RoughnessValue = 0.3f;

		// Generic property storage for extensibility
		std::unordered_map<std::string, float> m_CustomFloats;
		std::unordered_map<std::string, glm::vec3> m_CustomVec3s;
		std::unordered_map<std::string, glm::vec4> m_CustomVec4s;
		// Note: Mat4 properties typically not serialized (set at runtime)

		// Texture properties: maps uniform name to texture asset GUID
		std::unordered_map<std::string, Guid> m_TextureGuids;

		static constexpr uint64_t MATERIAL_MAGIC_VALUE{ iso8859ToBinary("E.MAT") };

		MaterialAssetData& operator>>(std::ostream& outp);
		MaterialAssetData const& operator>>(std::ostream& outp) const;

		//rets remaining buffer size
		std::uint64_t DumpToMemory(char* buff, std::uint64_t buffer_size) const;
	};
}

#endif