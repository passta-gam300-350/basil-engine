/******************************************************************************/
/*!
\file   ManagedTime.hpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3451 / UXG3450
\date   2026/02/01
\brief This file contains the declaration for the ManagedTime class, which
is responsible for managing time-related functions from managed code (C#).

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MANAGED_TIME_HPP
#define MANAGED_TIME_HPP
#include "Engine.hpp"

class ManagedTime
{
public:
	static float GetDeltaTime()
	{
		return static_cast<float>(Engine::GetLastDeltaTime());
	}
};
#endif// MANAGED_TIME_HPP
