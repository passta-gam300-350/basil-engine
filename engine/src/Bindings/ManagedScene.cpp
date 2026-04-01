/******************************************************************************/
/*!
\file   ManagedScene.cpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3451 / UXG3450
\date   2026/02/01
\brief This file contains the definition for the ManagedScene class, which
is responsible for managing scenes from managed code (C#).

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Bindings/ManagedScene.hpp"

#include "Engine.hpp"
#include "mono_include_pkg.h"
#include "Scene/Scene.hpp"
void ManagedScene::LoadSceneIndex(int32_t index)
{

	auto& Engine = ::Engine::Instance();

	Engine.GetSceneRegistry().RequestSceneChange(index);
}
void ManagedScene::LoadSceneName(MonoString* name)
{


	auto& Engine = ::Engine::Instance();
	char* name_cstr = mono_string_to_utf8(name);

	std::string path = std::string{ Engine.getWorkingDir() } + "/" + std::string{ name_cstr };
	Engine.GetSceneRegistry().RequestSceneChange(path);

	mono_free(name_cstr);
}

int ManagedScene::GetCurrentSceneIndex()
{	
	auto& Engine = ::Engine::Instance();
	return Engine.GetSceneRegistry().GetCurrentIndex();

}

void ManagedScene::ExitApplication()
{

	Engine::ShouldExit();
}
