#include "Manager/AssetManager.hpp"
#include <Manager/ResourceSystem.hpp>
#include <windows.h>
#include <filesystem>
#include <iostream>
#include <Utility/StringConversion.hpp>

#include <importer/importer.hpp>
#include <descriptors/material.hpp>
#include <descriptors/audio.hpp>
#include <glm/glm.hpp>
#include "Screens/EditorMain.hpp"
#include <ranges>

// YAML serialization support for BasicIndexedGuid
namespace YAML {
	template<>
	struct convert<rp::BasicIndexedGuid> {
		static Node encode(const rp::BasicIndexedGuid& rhs) {
			Node node;
			node["guid"] = rhs.m_guid.to_hex();
			node["typeindex"] = rhs.m_typeindex;
			return node;
		}

		static bool decode(const Node& node, rp::BasicIndexedGuid& rhs) {
			if (!node.IsMap() || !node["guid"] || !node["typeindex"]) {
				return false;
			}
			rhs.m_guid = rp::Guid::to_guid(node["guid"].as<std::string>());
			rhs.m_typeindex = node["typeindex"].as<std::uint64_t>();
			return true;
		}
	};
}

template<typename K, typename V>
static std::map<V, K> reverseMap(const std::map<K, V>& m) {
	std::map<V, K> r;
	for (const auto& kv : m)
		r[kv.second] = kv.first;
	return r;
}

static std::map<rp::BasicIndexedGuid, std::string> reverseMapGuid(const std::map<std::string, rp::BasicIndexedGuid>& m) {
	std::map<rp::BasicIndexedGuid, std::string> r;
	for (const auto& kv : m)
		r[kv.second] = kv.first;
	return r;
}

bool hideFolder(const std::wstring& path) {
	DWORD attrs = GetFileAttributesW(path.c_str());
	if (attrs == INVALID_FILE_ATTRIBUTES) return false;

	// Add the hidden attribute
	return SetFileAttributesW(path.c_str(), attrs | FILE_ATTRIBUTE_HIDDEN);
}

std::string AssetManager::getParentPath(std::string const& path) {
	return std::filesystem::path(path).parent_path().make_preferred().string();
}

std::string AssetManager::normalizePath(const std::string& Path) {
	std::filesystem::path p(Path);

	// `lexically_normal` cleans up redundant separators, "." and ".."
	std::filesystem::path normalized = p.lexically_normal();

	// `make_preferred` converts to the platform's preferred separator
	return normalized.make_preferred().string();
}

std::string const& AssetManager::GoToParentDirectory() {
	if (m_CurrentPath != m_RootPath) {
		m_CurrentPath = getParentPath(m_CurrentPath);
	}
	return m_CurrentPath;
}
std::string const& AssetManager::GoToSubDirectory(std::string const& sub) {
	m_CurrentPath = sub;
	return m_CurrentPath;
}

std::vector<std::string> AssetManager::GetSubDirectories() {
	std::vector<std::string> directory;
	for (const auto& entry : std::filesystem::directory_iterator(m_CurrentPath)) {
		if (entry.is_directory()) {
			directory.emplace_back(entry.path().string());
		}
	}
	return directory;
}

AssetManager::AssetManager(std::string const& root_dir, std::string const& import_dir)
	: m_AssetNameGuid{}, m_RootPath{ normalizePath(root_dir) }, m_CurrentPath{ m_RootPath }, m_ImportedAssetPath{ import_dir.empty() ? m_RootPath : normalizePath(import_dir) }, m_IndexingWorker{ &AssetManager::FileIndexingWorkerLoop, std::ref(*this) } {
	m_LastNotificationTime = std::chrono::steady_clock::now();
	m_NeedsRescan = false;
	if (!std::filesystem::exists(m_ImportedAssetPath)) {
		std::filesystem::create_directories(m_ImportedAssetPath);
	}
	if (!std::filesystem::exists(m_RootPath)) {
		std::filesystem::create_directories(m_RootPath);
	}
	hideFolder(string_to_wstring(m_ImportedAssetPath));
}

rp::BasicIndexedGuid AssetManager::ResolveAssetGuid(std::string const& name) {
	return m_AssetNameGuid.find(name) != m_AssetNameGuid.end() ? m_AssetNameGuid[name] : rp::null_indexed_guid;
}
std::string AssetManager::ResolveAssetName(rp::BasicIndexedGuid guid) {
	auto it = m_AssetReverse.find(guid);
	if (it == m_AssetReverse.end()) {
		m_AssetReverse = reverseMapGuid(m_AssetNameGuid); //lazy construction
		it = m_AssetReverse.find(guid);
	}
	else if (m_AssetNameGuid.find(it->second) == m_AssetNameGuid.end()) {
		m_AssetReverse.erase(it);
		it = m_AssetReverse.end();
	}
	return it != m_AssetReverse.end() ? m_AssetReverse[guid] : std::string{};
}

std::string AssetManager::getFileExtension(std::string const& file) {
	std::filesystem::path path(file);
	return path.extension().string();
}

std::vector<std::string> AssetManager::GetAssetTypeNames(ResourceType ty) {
	std::vector<std::string> asstype{};
	for (auto [name, typed] : m_AssetNameGuid) {
		if (typed.m_typeindex == ty) {
			asstype.emplace_back(name);
		}
	}
	return asstype;
}

rp::BasicIndexedGuid AssetManager::ImportAsset(std::string const& rdesc) {
	auto importertype{ rp::ResourceTypeImporterRegistry::GetDescriptorImporterType(rdesc) };
	auto biguid{ rp::ResourceTypeImporterRegistry::GetDescriptorGuid(rdesc) };
	auto file_path{ normalizePath(m_ImportedAssetPath + "/" + biguid.m_guid.to_hex() + rp::ResourceTypeImporterRegistry::GetImporterSuffix(importertype)) };
	if (ResourceSystem::Instance().m_MappedIO.find(file_path) != ResourceSystem::Instance().m_MappedIO.end()) {
		ResourceSystem::Instance().m_MappedIO.erase(file_path); //unsafe
	}
	rp::ResourceTypeImporterRegistry::Import(importertype, rdesc, file_path);
	rp::ResourceTypeImporterRegistry::GetDescriptorGuid(rdesc);
	m_AssetNameGuid.emplace(rp::ResourceTypeImporterRegistry::GetDescriptorName(rdesc), biguid);
	ResourceSystem::FileEntry fentry{};
	fentry.m_Guid = biguid.m_guid;
	fentry.m_Path = file_path;
	fentry.m_Size = std::filesystem::file_size(fentry.m_Path);
	ResourceSystem::Instance().m_FileEntries.emplace(fentry.m_Guid, fentry);
	return biguid;
}

//this might cause issues if there are too many directories cos of recursion
std::vector<rp::BasicIndexedGuid> AssetManager::ImportAssetDirectory(std::string const& dir) {
	std::vector<rp::BasicIndexedGuid> guids;
	for (const auto& entry : std::filesystem::directory_iterator(dir)) {
		if (entry.is_directory()) {
			auto subguids{ ImportAssetDirectory(entry.path().string()) };
			guids.insert(guids.end(), subguids.begin(), subguids.end());
		}
	}
	auto files = GetFiles(dir);
	for (auto it = files.first; it != files.second; ++it) {
		guids.emplace_back(ImportAsset(it->second));
	}
	return guids;
}

void AssetManager::CreateMaterialDescriptor(std::string const& material_name) {
	// 1. Create descriptor with defaults
	MaterialDescriptor matDesc{};

	// 2. Set base properties
	matDesc.base.m_guid = rp::Guid::generate();
	matDesc.base.m_name = material_name;
	matDesc.base.m_importer = ".material";
	matDesc.base.m_importer_type = rp::utility::type_hash<MaterialDescriptor>::value();
	matDesc.base.m_source = ""; // No source file for manually created materials

	// 3. Set material defaults (neutral gray, non-metallic)
	matDesc.material.vert_name = "main_pbr.vert";
	matDesc.material.frag_name = "main_pbr.frag";
	matDesc.material.material_name = material_name;
	matDesc.material.albedo = glm::vec3(0.8f, 0.8f, 0.8f);
	matDesc.material.metallic = 0.0f;
	matDesc.material.roughness = 0.5f;
	matDesc.material.blend_mode = 0; // Opaque by default

	matDesc.material.texture_properties["u_AOMap"];
	matDesc.material.texture_properties["u_RoughnessMap"];
	matDesc.material.texture_properties["u_DiffuseMap"];
	matDesc.material.texture_properties["u_EmissiveMap"];
	matDesc.material.texture_properties["u_NormalMap"];
	matDesc.material.texture_properties["u_HeightMap"];
	matDesc.material.texture_properties["u_SpecularMap"];
	matDesc.material.texture_properties["u_MetallicMap"];

	// 4. Save descriptor to .desc file in current directory
	std::string desc_path = normalizePath(m_CurrentPath + "/" + material_name + ".desc");

	// Check if file already exists
	int counter = 1;
	std::string final_path = desc_path;
	while (std::filesystem::exists(final_path)) {
		final_path = normalizePath(m_CurrentPath + "/" + material_name + std::to_string(counter) + ".desc");
		counter++;
	}

	// Serialize to YAML using the correct serializer
	rp::serialization::yaml_serializer::serialize(matDesc, final_path);

	// 5. Add to file list
	{
		std::lock_guard lg{ m_DescriptorListMtx };
		m_FileList.emplace(m_CurrentPath, final_path);
	}

	// 6. Automatically import/compile the material
	ImportAsset(final_path);
}

void AssetManager::LoadImportSettings(std::string const& is)
{
	if (!m_InspectedDescriptor) {
		m_InspectedDescriptor.reset(new rp::DescriptorWrapper{ rp::ResourceTypeImporterRegistry::LoadDescriptor(is) });
		m_InspectedDescriptorPath = is;
	}
}

void AssetManager::UnloadImportSetting(std::string const& is)
{
	if (m_InspectedDescriptor) {
		rp::ResourceTypeImporterRegistry::Serialize(m_InspectedDescriptor->m_desc_importer_hash, "yaml", is.empty() ? m_InspectedDescriptorPath : is, *m_InspectedDescriptor);
		m_InspectedDescriptor.reset(nullptr);
		m_InspectedDescriptorPath.clear();
	}
}

void AssetManager::ClearImportSetting()
{
	if (m_InspectedDescriptor) {
		m_InspectedDescriptor.reset(nullptr);
		m_InspectedDescriptorPath.clear();
	}
}

rp::DescriptorWrapper& AssetManager::GetImportSettings()
{
	return *m_InspectedDescriptor;
}

std::string AssetManager::GetImportSettingsPath()
{
	return m_InspectedDescriptorPath;
}

void AssetManager::ExportAssetList() {
	std::string filename{ m_ImportedAssetPath + "/" + std::string(cx_AssetListFilename.begin(),cx_AssetListFilename.end()) };
	rp::serialization::yaml_serializer::serialize(m_AssetNameGuid, filename);
}

void AssetManager::ImportAssetList() {
	std::string assetfilename = m_ImportedAssetPath + "/" + std::string(cx_AssetListFilename.begin(), cx_AssetListFilename.end());
	if (!std::filesystem::exists(assetfilename))
		return;
	m_AssetNameGuid = rp::serialization::yaml_serializer::deserialize<std::map<std::string, rp::BasicIndexedGuid>>(assetfilename);
	for (auto [name, typed] : m_AssetNameGuid) {
		ResourceSystem::FileEntry fentry{};
		fentry.m_Guid = typed.m_guid;
		fentry.m_Path = m_ImportedAssetPath + "/" + typed.m_guid.to_hex() + rp::ResourceTypeImporterRegistry::GetResourceExt(typed.m_typeindex);
		fentry.m_Size = std::filesystem::file_size(fentry.m_Path);
		ResourceSystem::Instance().m_FileEntries.emplace(typed.m_guid, fentry);
	}
}

void AssetManager::FileIndexingWorkerLoop() {
	HANDLE hDir = CreateFileW(string_to_wstring(m_RootPath).c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
	OVERLAPPED overlapped{};
	overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	if (hDir == INVALID_HANDLE_VALUE) {
		std::cerr << "Failed to open directory : " << m_RootPath << "\n";
		return;
	}

	std::string str = std::filesystem::path(m_RootPath + "/../.imports").make_preferred().string();

	rp::utility::working_path() = m_RootPath;
	rp::utility::output_path() = str;

	ImportAssetList();

	try {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(m_RootPath, std::filesystem::directory_options::follow_directory_symlink)) {
			if (entry.is_directory()) {
				continue;
			}
			else {
				std::string desc_name = entry.path().string();
				std::string dir_path = getParentPath(desc_name);
				std::string ext_name = getFileExtension(desc_name);

				// Handle .prefab files separately - they're self-contained assets
				if (ext_name == ".prefab") {
					std::lock_guard lg{ m_DescriptorListMtx };
					m_FileList.emplace(dir_path, desc_name);
					continue;
				}

				desc_name = desc_name.substr(0, desc_name.find_last_of(".")) + ".desc";
				if (ext_name == ".texture" || ext_name == ".mesh" || ext_name == ".desc" || ext_name == ".mtl" || ext_name == ".audio") {
					continue;
				}
				std::lock_guard lg{ m_DescriptorListMtx };
				if (!std::filesystem::exists(desc_name)) {
					rp::ResourceTypeImporterRegistry::CreateDefaultDescriptor(entry.path().string(), m_RootPath);
				}
				m_FileList.emplace(dir_path, desc_name);
			}
		}

		// Second pass: Register standalone .desc files (materials without source files)
		for (const auto& entry : std::filesystem::recursive_directory_iterator(m_RootPath, std::filesystem::directory_options::follow_directory_symlink)) {
			if (entry.is_directory()) continue;

			std::string filepath = entry.path().string();
			std::string ext = getFileExtension(filepath);

			// Only process .desc files
			if (ext == ".desc") {
				// Check if this .desc has a corresponding source file
				std::string base_name = filepath.substr(0, filepath.find_last_of("."));
				bool has_source_file = false;

				// Check common source file extensions
				std::vector<std::string> source_exts = {".png", ".jpg", ".jpeg", ".fbx", ".obj", ".gltf", ".glb", ".wav", ".mp3", ".ogg", ".flac"};
				for (auto const& src_ext : source_exts) {
					if (std::filesystem::exists(base_name + src_ext)) {
						has_source_file = true;
						break;
					}
				}

				// Only register if it has NO source file (standalone descriptor)
				if (!has_source_file) {
					std::string dir_path = getParentPath(filepath);
					std::lock_guard lg{ m_DescriptorListMtx };
					m_FileList.emplace(dir_path, filepath);
				}
			}
		}
	}
	catch (const std::filesystem::filesystem_error& e) {
		std::cerr << "Filesystem error: " << e.what() << "\n";
	}

	char buffer[8192]; // 8KB buffer to handle multiple file notifications
	DWORD bytesReturned;

	while (!m_ShouldClose) {
		BOOL success = ReadDirectoryChangesW(hDir, &buffer, sizeof(buffer), TRUE, // watch subdirectories
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
			&bytesReturned, &overlapped, nullptr);
		if (!success && GetLastError() != ERROR_IO_PENDING) {
			// handle error
		}
		DWORD waitStatus = WaitForSingleObject(overlapped.hEvent, 1000); // 1s timeout
		if (waitStatus == WAIT_OBJECT_0) {
			DWORD bytes;
			if (!GetOverlappedResult(hDir, &overlapped, &bytes, FALSE)) {
				std::cerr << "GetOverlappedResult failed, error: " << GetLastError() << "\n";
				ResetEvent(overlapped.hEvent);
				continue;
			}
			// Check for buffer overflow
			if (bytes == 0) {
				std::cerr << "Warning: Directory change notification buffer overflow. Some file changes may have been missed.\n";
				ResetEvent(overlapped.hEvent);
				continue;
			}
			// process notifications in buffer
			FILE_NOTIFY_INFORMATION* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
			do {
				std::wstring filename(fni->FileName, fni->FileNameLength / sizeof(WCHAR));
				std::string nfile{ normalizePath(GetRootPath() + "/" + normalizePath(wstring_to_string(filename))) };

				// Update last notification time
				m_LastNotificationTime = std::chrono::steady_clock::now();

				// Skip directories early
				if (std::filesystem::exists(nfile) && std::filesystem::is_directory(nfile)) {
					if (fni->NextEntryOffset == 0) break;
					fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
						reinterpret_cast<BYTE*>(fni) + fni->NextEntryOffset);
					continue;
				}

				std::string file_ext{ getFileExtension(nfile) };

				// Skip .desc files early (they're generated, not source assets)
				if (file_ext == ".desc") {
					if (fni->NextEntryOffset == 0) break;
					fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
						reinterpret_cast<BYTE*>(fni) + fni->NextEntryOffset);
					continue;
				}

				std::string dir_path{};
				std::string descriptor_filepath{};
				switch (fni->Action) {
				case FILE_ACTION_MODIFIED:
				case FILE_ACTION_ADDED:
					if (fni->Action == FILE_ACTION_MODIFIED) {
						std::wcout << L"Modified: " << filename << "\n";
					}
					else {
						std::wcout << L"New file: " << filename << "\n";
					}
					if (file_ext.empty()) {
						break;
					}

					// Handle .prefab file modifications
					if (file_ext == ".prefab") {
						std::wcout << L"Prefab changed: " << filename << "\n";
						std::lock_guard lg{ m_ChangedPrefabsMtx };
						// Check if not already in the list
						if (std::find(m_ChangedPrefabs.begin(), m_ChangedPrefabs.end(), nfile) == m_ChangedPrefabs.end()) {
							m_ChangedPrefabs.push_back(nfile);
						}
						dir_path = getParentPath(nfile);
						{
							std::lock_guard lg_file{ m_DescriptorListMtx };
							m_FileList.emplace(dir_path, nfile);
						}
						break;
					}

					// Mark that we need a rescan after quiet period
					m_NeedsRescan = true;
					descriptor_filepath = nfile.substr(0, nfile.find_last_of(".")) + ".desc";
					dir_path = getParentPath(nfile);
					if (!std::filesystem::exists(descriptor_filepath)) {
						rp::ResourceTypeImporterRegistry::CreateDefaultDescriptor(nfile);
						{
							std::lock_guard lg{ m_DescriptorListMtx };
							m_FileList.emplace(dir_path, descriptor_filepath);
						}
					}
					break;
				case FILE_ACTION_REMOVED:
					std::wcout << L"Removed: " << filename << "\n";
					if (file_ext.empty()) {
						break;
					}
					dir_path = getParentPath(nfile);
					descriptor_filepath = nfile.substr(0, nfile.find_last_of(".")) + ".desc";
					if (std::filesystem::exists(descriptor_filepath)) {
						std::filesystem::remove(descriptor_filepath);
						{
							std::lock_guard lg{ m_DescriptorListMtx };
							auto files = GetFiles(dir_path);
							for (auto it = files.first; it != files.second; ++it) {
								if (normalizePath(it->second) == nfile) {
									m_FileList.erase(it); // erase just this one
									break;
								}
							}
						}
					}
					break;
				case FILE_ACTION_RENAMED_OLD_NAME:
					std::wcout << L"Renamed from: " << filename << "\n";
					break;
				case FILE_ACTION_RENAMED_NEW_NAME:
					std::wcout << L"Renamed to: " << filename << "\n";
					break;
				}
				if (fni->NextEntryOffset == 0) break;
				fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
					reinterpret_cast<BYTE*>(fni) + fni->NextEntryOffset);
			} while (true);
			// Reset the manual-reset event for next notification batch
			ResetEvent(overlapped.hEvent);
		}

		// Check if we need to rescan after quiet period (2 seconds)
		auto now = std::chrono::steady_clock::now();
		auto time_since_last = std::chrono::duration_cast<std::chrono::seconds>(
			now - m_LastNotificationTime.load()).count();

		if (m_NeedsRescan && time_since_last >= 2) {
			// Quiet for 2 seconds - do rescan to catch any dropped notifications
			RescanDirectory();
			m_NeedsRescan = false;
		}
	}

	CloseHandle(hDir);
}

void AssetManager::RescanDirectory() {
	try {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(m_RootPath, std::filesystem::directory_options::follow_directory_symlink)) {
			if (entry.is_directory()) {
				continue;
			}

			std::string file_path = entry.path().string();
			std::string ext_name = getFileExtension(file_path);
			std::string desc_name = file_path.substr(0, file_path.find_last_of(".")) + ".desc";

			// Skip files we don't process
			if (ext_name == ".texture" || ext_name == ".mesh" || ext_name == ".desc" || ext_name == ".mtl") {
				continue;
			}

			// Skip if descriptor already exists
			if (std::filesystem::exists(desc_name)) {
				continue;
			}

			// This file is missing a descriptor - create one
			std::string dir_path = getParentPath(file_path);
			rp::ResourceTypeImporterRegistry::CreateDefaultDescriptor(file_path);
			{
				std::lock_guard lg{ m_DescriptorListMtx };
				m_FileList.emplace(dir_path, file_path);
			}
				// Log recovered file
				std::filesystem::path p(file_path);
				std::wcout << L"Recovered: " << p.filename().wstring() << L"\n";
			}
		}
	catch (const std::filesystem::filesystem_error& e) {
		std::cerr << "Rescan error: " << e.what() << "\n";
	}
}

std::vector<std::string> AssetManager::GetAndClearChangedPrefabs() {
	std::lock_guard lg{ m_ChangedPrefabsMtx };
	std::vector<std::string> changed = std::move(m_ChangedPrefabs);
	m_ChangedPrefabs.clear();
	return changed;
}