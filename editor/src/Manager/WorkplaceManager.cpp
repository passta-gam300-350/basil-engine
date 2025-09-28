#include "Manager/WorkplaceManager.hpp"

#include <filesystem>
#include <fstream>


void WorkplaceManager::AddWorkplace(const std::string& name, const std::string& path)
{
	UUID<128> id = UUID<128>::Generate();
	workplaces.emplace(name, Workplace{id, path, std::time(nullptr)});
}

void WorkplaceManager::RemoveWorkplace(const std::string& path)
{
	for (auto it = workplaces.begin(); it != workplaces.end(); ++it)
	{
		if (it->second.path == path)
		{
			if (currentWorkplace == it)
			{
				currentWorkplace = workplaces.end();
			}
			workplaces.erase(it);
			break;
		}
	}
}

WorkplaceManager::WorkplaceManager() : authProjectFile("../editor/.projects")
{
	Prepare();
}


WorkplaceManager& WorkplaceManager::GetInstance()
{
	static WorkplaceManager instance;
	return instance;
}

WorkplaceManager::WorkspaceList::iterator WorkplaceManager::GetCurrentWorkplace() const
{
	return currentWorkplace;
}

void WorkplaceManager::SetCurrentWorkplace(const std::string& name)
{
	auto it = workplaces.find(name);
	if (it != workplaces.end())
	{
		currentWorkplace = it;
	}
}

void WorkplaceManager::SetupWorkspace(Workplace& wp)
{

	std::ofstream configProjectRoot{ authProjectFile, std::ios::app };
	if (!configProjectRoot)
	{
		// Handle error
		return;
	}
	// Create new if DNE else amend
	configProjectRoot << wp.path << "\n";
	configProjectRoot.close();

	std::string dir = wp.path;
	std::filesystem::path fp(dir);
	if (!dir.empty())
	{
		// Ensure the directory exists
		if (!std::filesystem::exists(dir))
		{
			std::filesystem::create_directories(dir);
		}
		// Update last opened time
		wp.lastOpened = std::time(nullptr);

		// Create subdirectories
		std::filesystem::create_directories(dir + "/schemas");
		std::filesystem::create_directories(dir + "/library");
		std::filesystem::create_directories(dir + "/assets");
		std::filesystem::create_directories(dir + "/assets/models");
		std::filesystem::create_directories(dir + "/config");
		std::filesystem::create_directories(dir + "/assets/scenes");
		std::filesystem::create_directories(dir + "/assets/scripts");

		// Create default files if they don't exist
		std::string marker = dir + "/.basil";
		if (!std::filesystem::exists(marker))
		{
			std::ofstream markerFile{ marker };
			// 1st line: uuid
			// 2st line: name
			// 3nd line: working directory
			markerFile << wp.id.ToString() << "\n";
			markerFile << fp.filename().string() << "\n";
			markerFile << dir << "\n";
			markerFile.close();
			

		}

		std::string projConfig = dir + "/config/project.json";
		if (!std::filesystem::exists(projConfig))
		{
			std::ofstream(projConfig) << "{\n\t\"project_name\": \"New Project\",\n\t\"version\": \"1.0.0\"\n}\n";
		}




	}

}

void WorkplaceManager::LoadWorkspace(std::string const& path)
{
	std::filesystem::path basilPath{ path };
	std::ifstream workspaceFile(path);
	if (!workspaceFile) return;

	std::string bin{};
	std::getline(workspaceFile, bin);
	std::string name{};
	std::getline(workspaceFile, name);
	std::string workingDir{};
	std::getline(workspaceFile, workingDir);
	workspaceFile.close();

	std::filesystem::path fp(workingDir);
	
	if (!std::filesystem::exists(fp))
	{
		RemoveWorkplace(name);
		// edit .basil to remove this entry

		

		std::ofstream basilFile(path);
		basilFile << UUID<128>::Generate().ToString() << "\n";
		basilFile << name << "\n";
		basilFile << std::filesystem::absolute(basilPath.parent_path()).string() << "\n";
		basilFile.close();

	}



	std::ofstream configProjectRoot{ authProjectFile, std::ios::app };
	if (!configProjectRoot)
	{
		// Handle error
		return;
	}
	// Create new if DNE else amend
	configProjectRoot << std::filesystem::absolute(basilPath.parent_path()).string() << "\n";
	configProjectRoot.close();
	AddWorkplace(name, std::filesystem::absolute(basilPath.parent_path()).string());
	SetCurrentWorkplace(name);

	
}


void WorkplaceManager::Prepare()
{
	// Load the project file
	std::ifstream projectFile(authProjectFile);

	if (!projectFile) return;
	workplaces.clear();
	std::string line;
	while (std::getline(projectFile, line))
	{
		std::string check = line + "/.basil";
		if (!std::filesystem::exists(check) || !std::filesystem::exists(line))
		{
			continue; // Skip invalid entries
		}
		std::filesystem::path p{ line };
		AddWorkplace(p.filename().string(), p.string());

	}
	projectFile.close();
}
