#ifndef RESOURCE_DESCRIPTOR_TEXTURE
#define RESOURCE_DESCRIPTOR_TEXTURE

#include <rsc-ext/descriptor.hpp>
#include <tinyddsloader.h>

enum CompressionLevel {
	BC1,
	BC3,
	BC4,
	BC5
};

struct TextureDescriptor {
	std::string file_source;
	CompressionLevel compression
};

#endif