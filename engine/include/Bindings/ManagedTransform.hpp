/******************************************************************************/
/*!
\file   ManagedTransform.hpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the declaration for the ManagedTransform class, which
is responsible for managing and getting various transform-related functionalities in
the managed environment.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Ecs/ecs.h"
#ifndef ManagedTransform_HPP
#define ManagedTransform_HPP

class ManagedTransform
{
public:
	static void SetPosition(uint64_t handle, float x, float y, float z);
	static void SetRotation(uint64_t handle, float pitch, float yaw, float roll);
	static void SetScale(uint64_t handle, float x, float y, float z);

	static void GetPosition(uint64_t handle, float* x, float* y, float* z);
	static void GetRotation(uint64_t handle, float* pitch, float* yaw, float* roll);
	static void GetScale(uint64_t handle, float* x, float* y, float* z);

};
#endif // ManagedTransform_HPP