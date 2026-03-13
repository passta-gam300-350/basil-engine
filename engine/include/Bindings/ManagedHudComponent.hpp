/******************************************************************************/
/*!
\file   ManagedHudComponent.hpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3451 / UXG3450
\date   2026/02/01
\brief This file contains the declaration for the ManagedHudComponent class, which
is responsible for managing HUD components from managed code (C#).

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef ManagedHudComponent_HPP
#define ManagedHudComponent_HPP
#include <cstdint>

class ManagedHudComponent
{
public:

	static void GetPosition(uint64_t handle, float* out_x, float* out_y);
	static void SetPosition(uint64_t handle, float x, float y);

	static bool GetVisibility(uint64_t handle);
	static void SetVisibility(uint64_t handle, bool visible);

	static void GetSize(uint64_t handle, float* out_width, float* out_height);
	static void SetSize(uint64_t handle, float width, float height);

};

#endif
