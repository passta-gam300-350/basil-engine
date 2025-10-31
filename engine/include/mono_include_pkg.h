/******************************************************************************/
/*!
\file   mono_include_pkg.h
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file includes the most commonly used Mono headers for
glue generated source files.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MONO_INCLUDE_PKG_H
#define MONO_INCLUDE_PKG_H

//Objects
#include <mono/metadata/class.h>
#include <mono/metadata/object.h>
#include <mono/metadata/assembly.h>

// Debug headers
#include <mono/metadata/debug-helpers.h>

// Runtime headers
#include <mono/metadata/mono-config.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/environment.h>

// Garbage collector
#include <mono/metadata/mono-gc.h>

// Array, List, String etc
#include <mono/metadata/array.h>
#include <mono/metadata/list.h>
#include <mono/metadata/string.h>

// Reflection
#include <mono/metadata/reflection.h>

#endif	
