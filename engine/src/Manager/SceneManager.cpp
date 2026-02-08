/******************************************************************************/
/*!
\file   SceneManager.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Scene manager implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
//#include "Scene/Scene.hpp"
//#include "Manager/SceneManager.hpp"
//
//#include <iostream>
//
//#include "Manager/StringManager.hpp"
//
//
//SceneManager* SceneManager::_instance{};
//
//
//SceneManager& SceneManager::GetInstance()
//{
//	if (!_instance)
//	{
//		return *(_instance = new SceneManager{});
//	}
//	return *_instance;
//}
//
//void SceneManager::AddScene(Scene const& scene)
//{
//	scenes[scene.m_scene_id] = scene;
//}
//
//void SceneManager::RemoveScene(std::string const& sceneName)
//{
//	uint32_t name = StringManager::GetInstance().Intern_String(std::move(sceneName));
//	scenes.erase(name);
//}
//
//void SceneManager::init()
//{
//	std::cout << "SceneManager Init" << std::endl;
//}
//
//void SceneManager::update()
//{
//	
//}
//
//void SceneManager::exit()
//{
//	
//}
//
//
//Scene SceneManager::CreateEmpty(std::string const& name)
//{
//	Scene scene{name};
//	
//	return scene;
//}

