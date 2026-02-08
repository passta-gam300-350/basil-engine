/******************************************************************************/
/*!
\file   scene.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Native scene resource type

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef LIB_RESOURCE_CORE_NATIVE_SCENE_H
#define LIB_RESOURCE_CORE_NATIVE_SCENE_H

#include <rsc-core/guid.hpp>
#include "resource/utility.h"
#include "serialization/native_serializer.h"

struct SceneResourceData {
	Blob scene_data;
};

#endif