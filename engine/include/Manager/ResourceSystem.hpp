#ifndef RESOURCESYSTEM_HPP
#define RESOURCESYSTEM_HPP

#include <memory>
#include <jobsystem.hpp>
#include <unordered_map>
#include <serialisation/guid.h>

struct BasicResource {
	virtual void Release() {};
	virtual ~BasicResource() {
		Release();
	};
};

struct ResourceEntry {
	Resource::Guid m_guid;
	std::string	file_path;
};

struct ResourceSystem
{
private:
	static std::unique_ptr<ResourceSystem> m_Instance;
	std::unordered_map<Resource::Guid, std::shared_ptr<BasicResource>> m_Resources;
	std::unordered_map<Resource::Guid, std::string> m_ResourceFileList;

public:
	static void Init(std::string const& asset_list = {});
	static void Update();
	static void Exit();

private:
	static void Load(Resource::Guid guid);
	static void Load(std::string const& path);

public:
	static ResourceSystem& GetInstance();

	template <typename ResourceType>
	static std::shared_ptr<ResourceType> Get(Resource::Guid);
};

template<typename ResourceType> 
inline std::shared_ptr<ResourceType> ResourceSystem::Get(Resource::Guid guid)
{
	ResourceSystem& rs{ GetInstance() };
	if (rs.m_Resources.find(guid) != rs.m_Resources.end()) {
		Load(guid);
	}
	return std::static_pointer_cast<ResourceType>(rs.m_Resources[guid]);
}

#endif //!SCENEMANAGER_HPP
