#ifndef LIB_RESOURCE_CORE_NATIVE_TEXTURE_H
#define LIB_RESOURCE_CORE_NATIVE_TEXTURE_H

//#define TINYDDSLOADER_IMPLEMENTATION
#include <tinyddsloader.h>

#include <vector>
#include <fstream>
#include <string>
#include <glm/glm.hpp>

#include "resource/utility.h"
#include <rsc-core/guid.hpp>

using TextureResource = tinyddsloader::DDSFile;

struct TextureResourceData {
	Blob m_TexData;
};

#endif