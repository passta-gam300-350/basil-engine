#include "Manager/WorkplaceManager.hpp"


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


