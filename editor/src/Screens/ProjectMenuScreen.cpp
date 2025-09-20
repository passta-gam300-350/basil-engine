#include "Screens/ProjectMenuScreen.hpp"

#include <algorithm>
#include <iostream>
#include <__msvc_ostream.hpp>

#include "Editor.hpp"
#include "imgui_internal.h"
#include "glfw/glfw3.h"


bool ProjectMenuScreen::Activate()
{
	active = true;
	return true;
}

void ProjectMenuScreen::Close()
{
	active = false;
}


void ProjectMenuScreen::Show()
{
	
}

bool ProjectMenuScreen::isWindowClosed()
{
	return glfwWindowShouldClose(window);
}

void ProjectMenuScreen::init()
{
	// Set window title
	glfwSetWindowTitle(window, "Project Menu");
	active = true;
	// Get primary monitor and its video mode
	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
	// Set window size to 600x400 and center it on the screen
	glfwSetWindowSize(window, 1200, 400);
	glfwSetWindowPos(window, (mode->width - 1200) / 2, (mode->height - 400) / 2);
	// Set decoration on
	glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);

	// Add some dummy workspaces for demonstration
	// In a real application, you would load these from a configuration file or user settings
	AddWorkspace("Sample Project 1", "/path/to/project1", "2024-10-01 10:00 AM");
	AddWorkspace("Sample Project 2", "/path/to/project2", "2024-10-02 11:30 AM");
	AddWorkspace("Sample Project 3", "/path/to/project3", "2024-10-03 02:15 PM");

	AddWorkspace("Sample Project 1", "/path/to/project1", "2024-10-01 10:00 AM");
	AddWorkspace("Sample Project 2", "/path/to/project2", "2024-10-02 11:30 AM");
	AddWorkspace("Sample Project 3", "/path/to/project3", "2024-10-03 02:15 PM");

	AddWorkspace("Sample Project 1", "/path/to/project1", "2024-10-01 10:00 AM");
	AddWorkspace("Sample Project 2", "/path/to/project2", "2024-10-02 11:30 AM");
	AddWorkspace("Sample Project 3", "/path/to/project3", "2024-10-03 02:15 PM");

	AddWorkspace("Sample Project 1", "/path/to/project1", "2024-10-01 10:00 AM");
	AddWorkspace("Sample Project 2", "/path/to/project2", "2024-10-02 11:30 AM");
	AddWorkspace("Sample Project 3", "/path/to/project3", "2024-10-03 02:15 PM");

	AddWorkspace("Amprex Project Alpha", "/path/to/amprex_project_alpha", "2024-10-04 03:30 PM");

}

void ProjectMenuScreen::cleanup()
{
	
}

ProjectMenuScreen::ProjectMenuScreen(GLFWwindow* _window)
{
	window = _window;
}

void ProjectMenuScreen::render()
{
	if (!active) return;
	ImGuiWindowFlags bgFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	// Fill the entire window
	ImGui::SetNextWindowPos({});
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

	ImGui::Begin("Project Menu", nullptr, bgFlags);

	bool openPress, newPress;
	if (ImGui::BeginTable("##table", 3,
		ImGuiTableFlags_NoBordersInBody))
	{
	
		ImGui::TableSetupColumn("Search", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Btn1", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Btn2", ImGuiTableColumnFlags_WidthFixed);

		ImGui::TableNextRow();

		
		ImGui::TableSetColumnIndex(0);
		
		ImGui::SetNextItemWidth(-FLT_MIN); // << makes it fill the gap
		ImGui::InputTextWithHint("##search", "Search Projects...", projectSearchBuf, IM_ARRAYSIZE(projectSearchBuf));

		
		ImGui::TableSetColumnIndex(1);
		ImGui::PushID("OPEN_PROJECT_BTN");
		 openPress = ImGui::Button("Open Project");
		ImGui::PopID();

		ImGui::TableSetColumnIndex(2);
		ImGui::PushID("NEW_PROJECT_BTN");
		 newPress = ImGui::Button("New Project");
		ImGui::PopID();

		ImGui::EndTable();
	}


	if (openPress)
	{
		// Logic to open a project
		// For demonstration, we'll just switch to the main editor state
		Editor::GetInstance().ChangeState(EditorState::EDITOR_MAIN);
		
	}
	if (newPress)
	{
		// Logic to create a new project
		// For demonstration, we'll just switch to the main editor state
		Editor::GetInstance().ChangeState(EditorState::EDITOR_MAIN);
	}

	ImGui::BeginChild("##project_list", ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y-100), true);
	render_project_list();
	ImGui::EndChild();

	ImGui::PushID("PROJECT_EXIT_BTN");
	bool exit = ImGui::Button("Exit") | ImGui::IsKeyPressed(ImGuiKey_Escape);

	if (exit)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	ImGui::PopID();

	ImGui::End();
}





void ProjectMenuScreen::AddWorkspace(char const* name, const char* path, const char* lastOpened)
{
	WorkspaceDetailUI workspace;
	workspace.name = name;
	workspace.path = path;
	workspace.lastOpened = lastOpened;
	workspacesDetails.push_back(workspace);
}

void ProjectMenuScreen::ClearWorkspaces()
{
	workspacesDetails.clear();
}


void ProjectMenuScreen::render_project_list()
{
	int i = 0;
	if (ImGui::BeginTable("projects", 2 , ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersH))
	{
		// Left col: stretch (details), Right col: fixed (button)
		ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed);

		for (const auto& workspace : workspacesDetails)
		{
			std::string lowerWSName = workspace.name;
			std::ranges::transform(lowerWSName.begin(), lowerWSName.end(), lowerWSName.begin(), ::tolower);
			std::string lowerSearch = projectSearchBuf;
			std::ranges::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);
			if (projectSearchBuf[0] != '\0' && lowerWSName.find(lowerSearch) == std::string::npos)
			{
				continue; // Skip this project if it doesn't match the search
			}


			ImGui::TableNextRow();

			// Column 1: project details
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Name: %s", workspace.name.c_str());
			ImGui::Text("Path: %s", workspace.path.c_str());
			ImGui::Text("Last Opened: %s", workspace.lastOpened.c_str());
			

			// Column 2: action button	
			ImGui::TableSetColumnIndex(1);
			std::string btnID = "OPEN_PROJECT_BTN_" + workspace.name;
			ImGui::PushID(btnID.c_str());
			if (ImGui::Button(("Open##" + workspace.name + std::to_string(i++)).c_str()))
			{
				// handle opening this project
				std::cout << "Opening project: " << workspace.name << " at path: " << workspace.path << std::endl;
				Editor::GetInstance().Set_Working_Path(workspace.path.c_str());
				Editor::GetInstance().Set_Workspace_Name(workspace.name.c_str());
				Editor::GetInstance().ChangeState(EditorState::EDITOR_MAIN);
			}
			ImGui::PopID();



			
		}


		ImGui::EndTable();

		
	}
}
