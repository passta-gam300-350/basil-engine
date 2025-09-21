#ifndef WORKPLACEMANAGER_HPP
#define WORKPLACEMANAGER_HPP
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <uuid/uuid.hpp>

class WorkplaceManager
{
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
	static WorkplaceManager& GetInstance();
	void AddWorkplace(const std::string& name, const std::string& path);
	void RemoveWorkplace(const std::string& path);

	WorkspaceList::iterator GetCurrentWorkplace() const;
	void SetCurrentWorkplace(const std::string& name);
};


#endif // WORKPLACEMANAGER_HPP