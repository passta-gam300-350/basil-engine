/******************************************************************************/
/*!
\file   video.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Native video resource type

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef LIB_RESOURCE_CORE_NATIVE_VIDEO_H
#define LIB_RESOURCE_CORE_NATIVE_VIDEO_H

#include "resource/utility.h"
#include "serialization/native_serializer.h"

struct VideoResourceData {
    Blob m_VidData;
};

#endif
