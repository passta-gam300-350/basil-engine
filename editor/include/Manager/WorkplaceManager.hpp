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