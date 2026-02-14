/******************************************************************************/
/*!
\file   texture.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Texture descriptor definitions

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef RESOURCE_DESCRIPTOR_TEXTURE
#define RESOURCE_DESCRIPTOR_TEXTURE

#include <rsc-ext/descriptor.hpp>
#include "serialization/serializer.h"

enum class CompressionFormat : std::uint8_t {
	BC1_RGB4 = 71u, //4bit rgb, no alpha or 1 bit alpha
	BC3_RGBA8 = 77u, //full 8bit rgba
	BC4_R4 = 80u, //single channel for height maps and stuff
	BC5_RG8 = 83u //dual channel 8 bit, for normals
};

enum class TextureArchiveFormat : std::uint8_t {
	UNCOMPRESSED, 
	COMPRESSED //png, jpeg...
};

struct TextureDescriptor {
	rp::descriptor_base base;

	CompressionFormat compression{ CompressionFormat::BC3_RGBA8};
};

#endif