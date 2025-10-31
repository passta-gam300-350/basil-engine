#ifndef RESOURCE_DESCRIPTOR_TEXTURE
#define RESOURCE_DESCRIPTOR_TEXTURE

enum class Compression {
	BC1,
	BC3,
	BC4,
	BC5
};

struct TextureDescriptor {
	Compression compression_type;
};

#endif