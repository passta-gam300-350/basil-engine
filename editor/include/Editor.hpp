/******************************************************************************/
/*!
\file   Editor.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the declaration for the Editor class, which is the
main editor screen handling the viewport, entity management, and various editor panels.
It integrates with the rendering system and ECS to provide a functional editor environment.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef EDITOR_HPP
#define EDITOR_HPP

#include <memory>
#include <string>
#include <unordered_map>

struct GLFWwindow;

class Screen;

enum struct EditorState
{
#include "defines/EditorStates.def"
	EDITOR_NONE
};



struct EDITOR_CONFIG
{
	std::string project_workingDir;
	std::string workspace_name;
	std::string engine_configPath;
};

class Editor
{
	EDITOR_CONFIG configuration;
	GLFWwindow* window = nullptr;
	EditorState currentState = EditorState::EDITOR_NONE;
	std::unordered_map<EditorState, std::unique_ptr<Screen>> screens;

public:

	Editor() = default;
	~Editor() = default;

	static Editor& GetInstance();

	void ChangeState(EditorState newState);
	EditorState GetState() const { return currentState; }
	GLFWwindow* GetWindowPtr() { return window; }

	void Init(GLFWwindow* _win);
	void Update();
	void Render();
	void Cleanup();

	bool ShouldClose();

	EDITOR_CONFIG& GetConfig() { return configuration; }


	void Set_Working_Path(const char* path);
	void Set_Workspace_Name(const char* name);
	void Unload();
	void Load();
};


#endif // EDITOR_HPP