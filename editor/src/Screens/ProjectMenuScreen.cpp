#include "Screens/ProjectMenuScreen.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>


#include "Editor.hpp"
#include "imgui_internal.h"
#include "glfw/glfw3.h"
#include "Manager/WorkplaceManager.hpp"


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


	WorkplaceManager::GetInstance();

	auto list = WorkplaceManager::GetInstance().GetAllWorkplaces();

	for (const auto& wp : list)
	{
		AddWorkspace(wp.name.c_str(), wp.path.c_str(), std::asctime(std::localtime(&wp.lastOpened)));
	}



	

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
		std::string path{};
		bool openfile = fileService.OpenFileDialog(path);

		if (openfile)
		{
			WorkplaceManager::GetInstance().LoadWorkspace(path);
			Editor::GetInstance().Set_Workspace_Name(WorkplaceManager::GetInstance().GetCurrentWorkplace()->first.c_str());
			Editor::GetInstance().Set_Working_Path(WorkplaceManager::GetInstance().GetCurrentWorkplace()->second.path.c_str());
			Editor::GetInstance().ChangeState(EditorState::EDITOR_MAIN);
		}
		
		
	}
	if (newPress)
	{
		//// Logic to create a new project
		//// For demonstration, we'll just switch to the main editor state
		//std::string p{};
		//bool openFolder = fileService.OpenFolderDialog(p);
		//std::filesystem::path fp(p);

		//
		//if (openFolder)
		//{
		//	// Get the last part of the path as the project name
		//	std::string projectName = fp.filename().string();
		//	WorkplaceManager::GetInstance().AddWorkplace(projectName, p);
		//	WorkplaceManager::Workplace& wp = (WorkplaceManager::GetInstance().GetWorkplace(projectName));
		//	WorkplaceManager::GetInstance().SetupWorkspace(wp);

		//	
		//	Editor::GetInstance().ChangeState(EditorState::EDITOR_MAIN);
		//}

		newProjectModal = true;
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

	if (newProjectModal)
	{
		
		render_new_project_modal();
	
		ImGui::OpenPopup("Create a new project##MODAL_POP_NEW_PROJECT");
	}
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

void ProjectMenuScreen::render_new_project_modal()
{

	ImGui::SetNextWindowSize({ 800,0 });
	ImGui::SetNextWindowPos({ (ImGui::GetIO().DisplaySize.x - 800) * 0.5f, (ImGui::GetIO().DisplaySize.y - 200) * 0.5f });
	if (!ImGui::BeginPopupModal("Create a new project##MODAL_POP_NEW_PROJECT", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{

		return;
	}
	constexpr unsigned MAX_PATH_LEN = 2048;
	// Modal content
	static char projectNameBuf[MAX_PATH_LEN]{};
	static char projectPathBuf[MAX_PATH_LEN]{};
	ImGui::Text("Project Name:");
	ImGui::SameLine();
	ImGui::InputText("##PROJECT_INPUT_NAME", projectNameBuf, sizeof(projectNameBuf));

	ImGui::Text("Project Folder:");
	ImGui::SameLine();
	ImGui::InputText("##PROJECT_INPUT_FDR", projectPathBuf, sizeof(projectPathBuf));
	ImGui::SameLine();
	
	ImGui::PushID("BUTTON_BROWSE_FOLDER_NEW_PROJ");
	if (ImGui::Button("Browse"))
	{
		std::string p{};
		bool openFolder = fileService.OpenFolderDialog(p);
		std::filesystem::path fp(p);
		if (openFolder)
		{
			std::strncpy(projectPathBuf, p.c_str(), sizeof(projectPathBuf));
			projectPathBuf[sizeof(projectPathBuf) - 1] = '\0'; // Ensure null-termination
		}
	}
	ImGui::PopID();

	ImGui::Separator();
	ImGui::TextWrapped("Note: A new folder will be created inside the selected folder with the project name.");
	ImGui::Separator();
	ImGui::Text("Project Path: %s\\%s", projectPathBuf, projectNameBuf);
	ImGui::Separator();
	ImGui::PushID("MODAL_CREATE_PROJ_CANCEL");
	bool cancelClicked = ImGui::Button("Cancel");
	ImGui::PopID();
	ImGui::SameLine();
	ImGui::PushID("MODAL_PROJ_CREATE_GO");
	bool createClicked = ImGui::Button("Create");
	ImGui::PopID();

	if (cancelClicked)
		{
		newProjectModal = false;
		ImGui::CloseCurrentPopup();
		// Clear buffers
		std::ranges::fill(std::begin(projectNameBuf), std::end(projectNameBuf), 0);
		std::ranges::fill(std::begin(projectPathBuf), std::end(projectPathBuf), 0);
	}

	if (createClicked)
	{

		// Create the new project here
		std::string combi = std::string(projectPathBuf) + "\\" + std::string(projectNameBuf);
		WorkplaceManager::GetInstance().AddWorkplace(projectNameBuf, combi.c_str());
		WorkplaceManager::Workplace& wp = (WorkplaceManager::GetInstance().GetWorkplace(projectNameBuf));
		WorkplaceManager::GetInstance().SetupWorkspace(wp);

		std::time_t now = std::time(nullptr);
		// ISO 8601 format
		std::string iso = std::asctime(std::localtime(&now));
		iso.pop_back(); // remove trailing newline
		// Add to UI list
		AddWorkspace(projectNameBuf, combi.c_str(), iso.c_str());
		{
			newProjectModal = false;
			ImGui::CloseCurrentPopup();
			// Clear buffers
			std::ranges::fill(std::begin(projectNameBuf), std::end(projectNameBuf), 0);
			std::ranges::fill(std::begin(projectPathBuf), std::end(projectPathBuf), 0);
		}
	}


	ImGui::EndPopup();
	
	
}

