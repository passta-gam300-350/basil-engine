/******************************************************************************/
/*!
\file   ManagedScene.hpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3451 / UXG3450
\date   2026/02/01
\brief This file contains the declaration for the ManagedScene class, which
is responsible for managing scene-related functions from managed code (C#).

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MANAGEDSCENE_HPP
#define MANAGEDSCENE_HPP
#include <string>
#include <memory>
typedef struct _MonoString MonoString;

class ManagedScene
{
public:
	static void LoadSceneIndex(int32_t index);
	static void LoadSceneName(MonoString* name);
	static void ExitApplication();

};

#endif //!MANAGEDSCENE_HPP