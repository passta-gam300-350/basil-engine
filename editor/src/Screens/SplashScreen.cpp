#include "Screens/SplashScreen.hpp"

#include "Editor.hpp"
#include "../../../out/_dep/imgui-src/imgui.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"


SplashScreen::SplashScreen(GLFWwindow* _window) : Screen(_window)
{
	
}

void SplashScreen::init()
{

	// Set window title
	glfwSetWindowTitle(window, "Basil Editor - Splash Screen");
	active = true;

	// Get primary monitor and its video mode
	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
	// Set window size to 400x300 and center it on the screen
	glfwSetWindowSize(window, 400, 300);
	glfwSetWindowPos(window, (mode->width - 400) / 2, (mode->height - 300) / 2);
	// Set decoration off
	glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);

}


void SplashScreen::render()
{
	static ImGuiIO& io = ImGui::GetIO(); (void)io;
	if (!active) return;

	ImGuiWindowFlags splashScreenFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	// Fill the entire window
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(io.DisplaySize);
	ImGui::Begin("Splash Screen", nullptr, splashScreenFlags);
	// Center the text
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 textSize = ImGui::CalcTextSize("Basil Editor");
	ImGui::SetCursorPos(ImVec2((windowSize.x - textSize.x) * 0.5f, (windowSize.y - textSize.y) * 0.5f));
	ImGui::Text("Basil Editor");
	ImGui::End();


	if (timer > 0.0f)
	{
		timer -= io.DeltaTime;
	}
	else
	{
		Editor::GetInstance().ChangeState(EditorState::EDITOR_PROJECT);
	}

}


void SplashScreen::cleanup()
{
	
}


void SplashScreen::Show()
{
	active = true;
}

void SplashScreen::Close()
{
	active = false;
}

bool SplashScreen::isWindowClosed()
{
	return glfwWindowShouldClose(window);
}

bool SplashScreen::Activate()
{
	return active;
}
