#ifndef LIB_RESOURCE_CORE_NATIVE_SHADER_H
#define LIB_RESOURCE_CORE_NATIVE_SHADER_H

#include <string>
#include <glm/glm.hpp>
#include "serialisation/guid.h"

namespace Resource {
	struct ShaderAssetData {
		std::string m_Name;
		std::string m_FragPath;
		std::string m_VertPath;

		static constexpr uint64_t SHADER_MAGIC_VALUE{ iso8859ToBinary("E.SHDR") };

		ShaderAssetData& operator>>(std::ostream& outp);
		ShaderAssetData const& operator>>(std::ostream& outp) const;

		//rets remaining buffer size
		std::uint64_t DumpToMemory(char* buff, std::uint64_t buffer_size) const;
	};
}

#endif