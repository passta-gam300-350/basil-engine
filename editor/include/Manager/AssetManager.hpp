/******************************************************************************/
/*!
\file   AssetManager.hpp
\author Team PASSTA
		Chew Bangxin Steven (bangxinsteven.chew\@digipen.edu)

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
#ifndef EDITOR_ASSET_MANAGER_H
#define EDITOR_ASSET_MANAGER_H

#include <map>
#include <thread>
#include <mutex>
#include <rsc-ext/rp.hpp>
#include <chrono>

using ResourceType = std::uint64_t;

//future improvements
// TODO: do not recompute the subdirectories, store it and update using filewatcher
struct AssetManager {
	std::map<std::string, rp::BasicIndexedGuid> m_AssetNameGuid; //potentially unsafe
	std::map<rp::BasicIndexedGuid, std::string> m_AssetReverse; //reverse lookup

	std::unordered_multimap<std::string, std::string> m_FileList;
	std::string m_RootPath;
	std::string m_CurrentPath;
	std::string m_ImportedAssetPath;
	std::mutex m_DescriptorListMtx;
	std::unique_ptr<std::thread> m_IndexingWorker; //indexing file watcher thread, automatically creates descriptors

	std::unique_ptr<rp::DescriptorWrapper> m_InspectedDescriptor;
	std::string m_InspectedDescriptorPath;

	std::atomic<bool> m_ShouldClose;
	std::atomic<std::chrono::steady_clock::time_point> m_LastNotificationTime;
	std::atomic<bool> m_NeedsRescan;

	std::function<void(std::string const&)> m_ActivityCallback;

	// Prefab synchronization tracking
	std::vector<std::string> m_ChangedPrefabs;  ///< List of prefab file paths that have changed
	std::mutex m_ChangedPrefabsMtx;              ///< Mutex for thread-safe access to changed prefabs list

	static constexpr std::string_view cx_AssetListFilename{".assetlist"};

	AssetManager(std::string const& root_dir, std::string const& import_dir = {});
	AssetManager(AssetManager const&) = default;
	~AssetManager() {
		m_ShouldClose = true;
		if (m_IndexingWorker)
			m_IndexingWorker->join();
		ExportAssetList();
	}

	void InitWorkerLoop();
		
	rp::BasicIndexedGuid ResolveAssetGuid(std::string const&);
	std::string ResolveAssetName(rp::BasicIndexedGuid);

	void FileIndexingWorkerLoop();
	void RescanDirectory();

	rp::BasicIndexedGuid ImportAsset(std::string const&);
	std::vector<rp::BasicIndexedGuid> ImportAssetDirectory(std::string const&);

	// Create new material descriptor with defaults
	void CreateMaterialDescriptor(std::string const& material_name);

	//import settings for 1 active descriptor
	void LoadImportSettings(std::string const&);
	void UnloadImportSetting(std::string const& = {});
	void ClearImportSetting();
	rp::DescriptorWrapper& GetImportSettings();
	std::string GetImportSettingsPath();

	auto GetFiles(std::string const& dir) {
		return m_FileList.equal_range(dir);
	}

	using ResourceType = std::uint64_t;

	std::vector<std::string> GetAssetTypeNames(ResourceType);

	void ExportAssetList();

	void ImportAssetList();

	std::vector<std::string> GetSubDirectories();
	std::string const& GetCurrentPath() const {
		return m_CurrentPath;
	}
	std::string const& GetRootPath() const {
		return m_RootPath;
	}
	void SetCurrentPath(std::string const& path) {
		m_CurrentPath = path;
	}
	std::string const& GoToParentDirectory();
	static std::string getFileExtension(std::string const&);
	static std::string getParentPath(std::string const&);
	std::string const& GoToSubDirectory(std::string const&);
	static std::string normalizePath(const std::string& Path);

	// Prefab synchronization methods
	std::vector<std::string> GetAndClearChangedPrefabs();
};

#endif