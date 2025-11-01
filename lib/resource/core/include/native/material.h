#ifndef LIB_RESOURCE_CORE_NATIVE_MATERIAL_H
#define LIB_RESOURCE_CORE_NATIVE_MATERIAL_H

#include <string>
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

		static constexpr uint64_t MATERIAL_MAGIC_VALUE{ iso8859ToBinary("E.MAT") };

		MaterialAssetData& operator>>(std::ostream& outp);
		MaterialAssetData const& operator>>(std::ostream& outp) const;

		//rets remaining buffer size
		std::uint64_t DumpToMemory(char* buff, std::uint64_t buffer_size) const;
	};
}

#endif