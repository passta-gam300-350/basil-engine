/******************************************************************************/
/*!
\file   utility.h
\author Team PASSTA
		Chew Bangxin Steven (banxginsteven.chew@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/09
\brief    Declares the utilities used by the ecs

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef LIB_ECS_UTILITY_H
#define LIB_ECS_UTILITY_H

#ifdef _MSC_VER
#define STRONG_INLINE __forceinline
#elif __GNUC__
#define STRONG_INLINE inline __attribute__((always_inline))
#endif

#endif