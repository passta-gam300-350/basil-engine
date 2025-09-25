#ifndef EDITOR_HPP
#define EDITOR_HPP

#include <memory>
#include <string>
#include <unordered_map>

struct GLFWwindow;

class Screen;

enum struct EditorState
{
#include "defines/EditorStates.def",
	EDITOR_NONE
};



struct EDITOR_CONFIG
{
	std::string project_workingDir;
	std::string workspace_name;
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


	void Init(GLFWwindow* _win);
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