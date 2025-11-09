/******************************************************************************/
/*!
\file   bvh_logging.h
\author Team PASSTA
		Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/01
\brief  Logging and debug output utilities for BVH visualization

Provides stream operators for outputting BVH data structures (vec3, AABB) to console
for debugging and visualization purposes. Used by DumpInfo and DumpGraph functions.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once
#ifndef BVH_LOGGING_H
#define BVH_LOGGING_H

#include <iostream>
#include "BVH/bvh_math.h"
#include "shapes.h"

// Minimal logging for BVH - only vec3 and Aabb needed for DumpInfo/DumpGraph
namespace glm {
    std::ostream& operator<<(std::ostream& os, vec3 const& v);
}

std::ostream& operator<<(std::ostream& os, Aabb const& aabb);

#endif // BVH_LOGGING_H
