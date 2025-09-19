#ifndef EDITOR_HPP
#define EDITOR_HPP

#include <memory>
#include <unordered_map>

struct GLFWwindow;

class Screen;

enum struct EditorState
{
#include "defines/EditorStates.def",
	EDITOR_NONE
};


class Editor
{
	
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
};


#endif // EDITOR_HPP