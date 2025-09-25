#ifndef LIB_RESOURCE_CORE_NATIVE_TEXTURE_H
#define LIB_RESOURCE_CORE_NATIVE_TEXTURE_H

#include <vector>
#include <fstream>
#include <string>
#include <glm/glm.hpp>

#include "serialisation/guid.h"
#include <tinyddsloader.h>
#include <DirectXTex.h>

namespace Resource {

	using TextureAssetData = DirectX::ScratchImage;
	/*
	TextureAssetData load_dds_texture(std::uint32_t guid);
	TextureAssetData load_dds_texture(std::string const& file_name);*/
	TextureAssetData load_dds_texture_from_memory(const char* data);
}

#endif