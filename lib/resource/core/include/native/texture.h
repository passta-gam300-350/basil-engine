/******************************************************************************/
/*!
\file   texture.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Native texture resource type

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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

enum class TextureType {

};

struct TextureResourceData {
	
	Blob m_TexData;
};

#endif