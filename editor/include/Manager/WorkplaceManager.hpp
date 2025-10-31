/******************************************************************************/
/*!
\file   WorkplaceManager.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the declaration of the WorkplaceManager class, which
manages multiple workplaces (projects) within the editor.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef WORKPLACEMANAGER_HPP
#define WORKPLACEMANAGER_HPP
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <uuid/uuid.hpp>

class WorkplaceManager
{
	std::string authProjectFile = "../editor/.projects";
public:
	
	struct Workplace
	{
		UUID<128> id;
		std::string path;
		std::time_t lastOpened;
	};

	using WorkspaceList = std::unordered_map<std::string, Workplace>;
private:

	WorkspaceList workplaces;
	WorkspaceList::iterator currentWorkplace = workplaces.end();

public:
	WorkplaceManager();
	static WorkplaceManager& GetInstance();

	void Prepare();

	void AddWorkplace(const std::string& name, const std::string& path);
	void RemoveWorkplace(const std::string& path);

	WorkspaceList::iterator GetCurrentWorkplace() const;
	void SetCurrentWorkplace(const std::string& name);


	void LoadWorkspace(std::string const& path);


	void SetupWorkspace(Workplace& wp);

	Workplace& GetWorkplace(const std::string& name)
	{
		return workplaces.at(name);
	}

	struct WorkplaceMeta
	{
		std::string name;
		std::string path;
		std::time_t lastOpened;
	};
	std::vector<WorkplaceMeta> GetAllWorkplaces() const
	{
		std::vector<WorkplaceMeta> metas;
		for (const auto& [name, wp] : workplaces)
		{
			metas.push_back({ name, wp.path, wp.lastOpened });
		}
		return metas;
	}
};


#endif // WORKPLACEMANAGER_HPP