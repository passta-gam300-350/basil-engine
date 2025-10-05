//THIS IS A EDITOR STATE WRAPPER

#include "Editor.hpp"

#include <stdexcept>

#include "Engine.hpp"
#include "Screens/EditorMain.hpp"
#include "Screens/Screen.hpp"
#include "Screens/SplashScreen.hpp"
#include "GLFW/glfw3.h"
#include "Screens/ProjectMenuScreen.hpp"

void Editor::ChangeState(EditorState newState)
{
	if (!screens.contains(newState))
		throw std::invalid_argument{"Invalid editor state"};


	if (currentState != newState)
	{
		if (currentState != EditorState::EDITOR_NONE)
			screens[currentState]->cleanup();
		currentState = newState;
		screens[currentState]->init();
		
	}

	screens[currentState]->Activate();

	
	
}

Editor& Editor::GetInstance()
{
	static Editor instance;
	return instance;
}

void Editor::Init(GLFWwindow* _win)
{
	this->window = _win;
	// Initialize screens
	screens[EditorState::EDITOR_SPLASHSCREEN] = std::make_unique<SplashScreen>(window);
	screens[EditorState::EDITOR_MAIN] = std::make_unique<EditorMain>(window);
	screens[EditorState::EDITOR_PROJECT] = std::make_unique<ProjectMenuScreen>(window);


	

	ChangeState(EditorState::EDITOR_PROJECT);


}


void Editor::Update()
{
	screens[currentState]->update();
}


void Editor::Render()
{
	screens[currentState]->render();
}

void Editor::Cleanup()
{
	for (auto const& kv : screens) {
		kv.second->cleanup();
	}
}


bool Editor::ShouldClose()
{
	return screens[currentState]->isWindowClosed();
}


void Editor::Load()
{
	
}

void Editor::Unload()
{

}

void Editor::Set_Working_Path(const char* path)
{
	configuration.project_workingDir = path;
}

void Editor::Set_Workspace_Name(const char* name)
{
	configuration.workspace_name = name;
}

