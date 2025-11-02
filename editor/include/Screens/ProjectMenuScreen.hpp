/******************************************************************************/
/*!
\file   ProjectMenuScreen.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)
		Eirwen (c.lau\@digipen.edu)
		Hai Jie (haijie.w\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the declaration of the ProjectMenuScreen class, which
is an IMGUI-based screen for managing and selecting projects within the editor.
It provides an interface for users to create, open, and manage their projects.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef PROJECTMENUSCREEN_HPP
#define PROJECTMENUSCREEN_HPP

#include <string>
#include <vector>

#include "Screen.hpp"
#include "Service/FileService.hpp"

class ProjectMenuScreen : public Screen
{
	struct WorkspaceDetailUI
	{
		std::string name;
		std::string path;
		std::string lastOpened;
	};

	std::vector<WorkspaceDetailUI> workspacesDetails;
	bool newProjectModal = false;

public:
	FileService fileService;

	char projectSearchBuf[256] = "";

	void init() override;
	void update() override;
	void render() override;
	void cleanup() override;
	void Show() override;
	void Close() override;
	virtual bool isWindowClosed() override;
	bool Activate() override;
	ProjectMenuScreen(GLFWwindow* window);
	~ProjectMenuScreen() override = default;



	void render_project_list();
	void render_new_project_modal();

	void AddWorkspace(char const* name, const char* path, const char* lastOpened);
	void ClearWorkspaces();

};

#endif // PROJECTMENUSCREEN_HPP