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
