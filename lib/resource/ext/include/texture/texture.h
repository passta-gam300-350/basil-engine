#ifndef LIB_RESOURCE_EXT_TEXTURE_H
#define LIB_RESOURCE_EXT_TEXTURE_H

#include "descriptors/descriptors.h"

namespace Resource {
	void compile_texture(ResourceDescriptor const& rdesc);
	void init();
}

#endif