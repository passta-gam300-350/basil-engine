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

public:
	FileService fileService;

	char projectSearchBuf[256] = "";

	void init() override;
	void render() override;
	void cleanup() override;
	void Show() override;
	void Close() override;
	virtual bool isWindowClosed() override;
	bool Activate() override;
	ProjectMenuScreen(GLFWwindow* window);
	~ProjectMenuScreen() override = default;



	void render_project_list();

	void AddWorkspace(char const* name, const char* path, const char* lastOpened);
	void ClearWorkspaces();

};

#endif // PROJECTMENUSCREEN_HPP