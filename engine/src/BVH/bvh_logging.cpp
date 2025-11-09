/******************************************************************************/
/*!
\file   bvh_logging.cpp
\author Team PASSTA
		Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/01
\brief  Implementation of BVH logging and debug output operators

Implements stream output operators for BVH data structures to enable console
debugging and visualization of vec3 and AABB geometries.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "BVH/bvh_logging.h"
#include "BVH/bvh_math.h"
#include <iostream>

// Minimal logging implementation for BVH debugging
namespace {
    template <typename T>
    std::ostream& GenericVecWrite(std::ostream& os, T const& v)
    {
        for (int i = 0; i < T::length(); ++i) {
            os << v[i];
            if (i + 1 != T::length()) {
                os << ", ";
            }
        }
        return os;
    }
}

namespace glm {
    std::ostream& operator<<(std::ostream& os, vec3 const& v) { return GenericVecWrite(os, v); }
}

std::ostream& operator<<(std::ostream& os, Aabb const& aabb)
{
    os << aabb.min << ", " << aabb.max;
    return os;
}
