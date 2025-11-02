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
