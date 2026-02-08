/******************************************************************************/
/*!
\file   ManagedGameObjects.hpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3451 / UXG3450
\date   2026/02/01
\brief This file contains the declaration for the ManagedGameObjects class, which
is responsible for managing game objects from managed code (C#).

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MANAGED_GAMEOBJECTS_HPP
#define MANAGED_GAMEOBJECTS_HPP
#include <cstdint>

typedef struct _MonoString MonoString;
typedef std::int32_t mono_bool;
class ManagedGameObjects
{
public:
	static void SetVisible(uint64_t handle, mono_bool val);
	static bool GetVisible(uint64_t handle);
	static uint64_t FindByName(MonoString* name);
	static void DeleteGameObject(uint64_t handle);

};


#endif //! MANAGED_GAMEOBJECTS_HPP