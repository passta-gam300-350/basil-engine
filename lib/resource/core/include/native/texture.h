#ifndef LIB_RESOURCE_CORE_NATIVE_TEXTURE_H
#define LIB_RESOURCE_CORE_NATIVE_TEXTURE_H

#include <vector>
#include <fstream>
#include <string>
#include <glm/glm.hpp>

#include "serialisation/guid.h"
#include <tinyddsloader.h>

namespace Resource {

	using TextureResource = tinyddsloader::DDSFile;

	TextureResource load_dds_texture(std::uint32_t guid);
	TextureResource load_dds_texture(std::string const& file_name);

}

#endif