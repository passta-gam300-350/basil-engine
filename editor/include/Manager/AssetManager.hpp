#ifndef EDITOR_ASSET_MANAGER_H
#define EDITOR_ASSET_MANAGER_H

#include <map>
#include <thread>
#include <mutex>
#include <chrono>
#include <importer/importer.hpp>

//future improvements
// TODO: do not recompute the subdirectories, store it and update using filewatcher
struct ResourceTypeGuid {
	Resource::ResourceType m_Type;
	Resource::Guid m_Guid;
};

struct AssetManager {
	std::map<std::string, ResourceTypeGuid> m_AssetNameGuid; //potentially unsafe
	std::map<Resource::Guid, std::string> m_AssetReverse; //reverse lookup

	std::unordered_multimap<std::string, Resource::ResourceDescriptor> m_Descriptors;
	std::string m_RootPath;
	std::string m_CurrentPath;
	std::string m_ImportedAssetPath;
	std::thread m_IndexingWorker; //indexing file watcher thread, automatically creates descriptors
	std::mutex m_DescriptorListMtx;

	std::atomic<bool> m_ShouldClose;
	std::atomic<std::chrono::steady_clock::time_point> m_LastNotificationTime;
	std::atomic<bool> m_NeedsRescan;

	static constexpr std::string_view cx_AssetListFilename{".assetlist"};

	AssetManager(std::string const& root_dir, std::string const& import_dir = {});
	AssetManager(AssetManager const&) = default;
	~AssetManager() {
		m_ShouldClose = true;
		m_IndexingWorker.join();
		ExportAssetList();
	}
		
	Resource::Guid ResolveAssetGuid(std::string const&);
	std::string ResolveAssetName(Resource::Guid);

	void FileIndexingWorkerLoop();
	void RescanDirectory();

	void ImportAsset(Resource::ResourceDescriptor&);
	void ImportAssetDirectory(std::string const&);

	auto GetFiles(std::string const& dir) {
		return m_Descriptors.equal_range(dir);
	}

	std::vector<std::string> GetAssetTypeNames(Resource::ResourceType);

	void ExportAssetList();

	void ImportAssetList();

	std::vector<std::string> GetSubDirectories();
	std::string const& GetCurrentPath() const {
		return m_CurrentPath;
	}
	std::string const& GetRootPath() const {
		return m_RootPath;
	}
	std::string const& GoToParentDirectory();
	static std::string getFileExtension(std::string const&);
	static std::string getParentPath(std::string const&);
	std::string const& GoToSubDirectory(std::string const&);
	static std::string normalizePath(const std::string& Path);
};

#endif