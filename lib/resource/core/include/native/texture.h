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

	enum class TextureType : std::uint8_t {
		DIFFUSE_BITMAP,
		SPECULAR_BITMAP,
		METALLIC_BITMAP,
		ROUGHNESS_BITMAP,
		NORMAL_BITMAP,
		HEIGHT_BITMAP,
		EMISSIVE_BITMAP,
		AO_BITMAP,
		MAX_COUNT
	};

	using TextureAssetData = tinyddsloader::DDSFile;

	struct TextureAsset {
		static constexpr uint64_t TEXTURE_MAGIC_VALUE{ iso8859ToBinary("E.TEX") };
		DirectX::Blob m_TexData;

		TextureAsset& operator>>(std::ofstream&);
		TextureAsset const& operator>>(std::ofstream&) const;
	};
	std::vector<std::string>& GetTextureTypeVector();
	std::string GetTextureTypeName(TextureType i);
	TextureAssetData load_dds_texture_from_memory(const char* data);
}

#endif