#ifndef LIB_ECS_UTILITY_H
#define LIB_ECS_UTILITY_H

#ifdef _MSC_VER
#define STRONG_INLINE __forceinline
#elif __GNUC__
#define STRONG_INLINE inline __attribute__((always_inline))
#endif

#endif