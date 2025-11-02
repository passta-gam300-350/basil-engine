#ifndef RESOURCE_DESCRIPTOR_TEXTURE
#define RESOURCE_DESCRIPTOR_TEXTURE

#include <rsc-ext/descriptor.hpp>
#include "serialization/serializer.h"

enum CompressionFormat : std::uint8_t {
	BC1_RGB4 = 71u, //4bit rgb, no alpha or 1 bit alpha
	BC3_RGBA8 = 77u, //full 8bit rgba
	BC4_R4 = 80u, //single channel for height maps and stuff
	BC5_RG8 = 83u //dual channel 8 bit, for normals
};

struct TextureDescriptor {
	rp::descriptor_base base;

	std::string file_source;
	CompressionFormat compression{ CompressionFormat::BC3_RGBA8};
};

#endif