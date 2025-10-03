#include "Manager/AssetManager.hpp"
#include <Manager/ResourceSystem.hpp>
#include <windows.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>
#include <importer/importer_registry.hpp>
#include <descriptors/descriptor_registry.hpp>

namespace YAML {
	template<>
	struct convert<std::map<std::string, ResourceTypeGuid>> {
		static Node encode(const std::map<std::string, ResourceTypeGuid>& rhs) {
			Node node;
			for (const auto& kv : rhs) {
				node[kv.first]["guid"] = kv.second.m_Guid.to_hex_no_delimiter();
				node[kv.first]["type"] = Resource::GetResourceTypeName(kv.second.m_Type);
			}
			return node;
		}

		static bool decode(const Node& node, std::map<std::string, ResourceTypeGuid>& rhs) {
			if (!node.IsMap()) return false;
			rhs.clear();
			for (auto it = node.begin(); it != node.end(); ++it) {
				ResourceTypeGuid typed{};
				typed.m_Guid = Resource::Guid::to_guid(it->second["guid"].as<std::string>());
				typed.m_Type = Resource::GetResourceTypeFromName(it->second["type"].as<std::string>());
				rhs[it->first.as<std::string>()] = typed;
			}
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

static std::map<Resource::Guid, std::string> reverseMapGuid(const std::map<std::string, ResourceTypeGuid>& m) {
	std::map<Resource::Guid, std::string> r;
	for (const auto& kv : m)
		r[kv.second.m_Guid] = kv.first;
	return r;
}

bool hideFolder(const std::wstring& path) {
	DWORD attrs = GetFileAttributesW(path.c_str());
	if (attrs == INVALID_FILE_ATTRIBUTES) return false;

	// Add the hidden attribute
	return SetFileAttributesW(path.c_str(), attrs | FILE_ATTRIBUTE_HIDDEN);
}

std::wstring string_to_wstring(const std::string& str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	std::wstring wstr(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);
	return wstr;
}

std::string wstring_to_string(const std::wstring& wstr) {
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string utf8(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], size_needed, nullptr, nullptr);
	return utf8;
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
	if (!std::filesystem::exists(m_ImportedAssetPath)) {
		std::filesystem::create_directories(m_ImportedAssetPath);
	}
	if (!std::filesystem::exists(m_RootPath)) {
		std::filesystem::create_directories(m_RootPath);
	}
	hideFolder(string_to_wstring(m_ImportedAssetPath));
}

Resource::Guid AssetManager::ResolveAssetGuid(std::string const& name) {
	return m_AssetNameGuid.find(name) != m_AssetNameGuid.end() ? m_AssetNameGuid[name].m_Guid : Resource::null_guid;
}
std::string AssetManager::ResolveAssetName(Resource::Guid guid) {
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

std::vector<std::string> AssetManager::GetAssetTypeNames(Resource::ResourceType ty) {
	std::vector<std::string> asstype{};
	for (auto [name, typed] : m_AssetNameGuid) {
		if (typed.m_Type == ty) {
			asstype.emplace_back(name);
		}
	}
	return asstype;
}

void AssetManager::ImportAsset(Resource::ResourceDescriptor& rdesc) {
	Resource::ImporterRegistry::SetImportDirectory(m_ImportedAssetPath);
	Resource::Import(rdesc);
	for (auto [type, entry] : rdesc.m_DescriptorEntries) {
		ResourceTypeGuid typed;
		typed.m_Guid = entry.m_Guid;
		typed.m_Type = type;
		m_AssetNameGuid.emplace(entry.m_Name, typed);
		ResourceSystem::FileEntry fentry{};
		fentry.m_Guid = entry.m_Guid;
		fentry.m_Path = m_ImportedAssetPath + "/" + entry.m_Guid.to_hex_no_delimiter() + "." + Resource::GetResourceTypeName(type);
		fentry.m_Size = std::filesystem::file_size(fentry.m_Path);
		ResourceSystem::Instance().m_FileEntries.emplace(entry.m_Guid, fentry);
	}
}

//this might cause issues if there are too many directories cos of recursion
void AssetManager::ImportAssetDirectory(std::string const& dir) {
	for (const auto& entry : std::filesystem::directory_iterator(dir)) {
		if (entry.is_directory()) {
			ImportAssetDirectory(entry.path().string());
		}
	}
	auto files = GetFiles(dir);
	for (auto it = files.first; it != files.second; ++it) {
		ImportAsset(it->second);
	}
}

void AssetManager::ExportAssetList() {
	std::ofstream ofs{ m_ImportedAssetPath + "/" + std::string(cx_AssetListFilename.begin(),cx_AssetListFilename.end()), std::ios::out };
	YAML::Node root{};
	root["asset list"] = m_AssetNameGuid;
	ofs << root;
}

void AssetManager::ImportAssetList() {
	std::string assetfilename = m_ImportedAssetPath + "/" + std::string(cx_AssetListFilename.begin(), cx_AssetListFilename.end());
	if (!std::filesystem::exists(assetfilename))
		return;
	YAML::Node root{ YAML::LoadFile(assetfilename) };
	if (!root["asset list"].IsNull()) {
		m_AssetNameGuid = root["asset list"].as<std::map<std::string, ResourceTypeGuid>>();
	}
	for (auto [name, typed] : m_AssetNameGuid) {
		ResourceSystem::FileEntry fentry{};
		fentry.m_Guid = typed.m_Guid;
		fentry.m_Path = m_ImportedAssetPath + "/" + typed.m_Guid.to_hex_no_delimiter() + "." + Resource::GetResourceTypeName(typed.m_Type);
		fentry.m_Size = std::filesystem::file_size(fentry.m_Path);
		ResourceSystem::Instance().m_FileEntries.emplace(typed.m_Guid, fentry);
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

	Resource::DescriptorRegistry::SetDescriptorRootDirectory(m_RootPath);
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
				desc_name = desc_name.substr(0, desc_name.find_last_of(".")) + ".desc";
				if (ext_name == ".texture" || ext_name == ".mesh" || ext_name == ".desc" || ext_name == ".mtl") {
					continue;
				}
				std::lock_guard lg{ m_DescriptorListMtx };
				if (!std::filesystem::exists(desc_name)) {
					Resource::ResourceDescriptor desc = Resource::ResourceDescriptor::MakeDescriptor(entry.path().string());
					if (desc.m_RawFileInfo.m_FileChecksumHash) {
						desc.SaveDescriptor();
						m_Descriptors.emplace(dir_path, desc);
					}
				}
				else {
					m_Descriptors.emplace(dir_path, Resource::ResourceDescriptor::LoadDescriptor(desc_name));
				}
			}
		}
	}
	catch (const std::filesystem::filesystem_error& e) {
		std::cerr << "Filesystem error: " << e.what() << "\n";
	}

	char buffer[1024];
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
			GetOverlappedResult(hDir, &overlapped, &bytes, FALSE);
			// process notifications in buffer
			FILE_NOTIFY_INFORMATION* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
			do {
				std::wstring filename(fni->FileName, fni->FileNameLength / sizeof(WCHAR));
				std::string nfile{ normalizePath(GetRootPath() + "/" + normalizePath(wstring_to_string(filename))) };
				std::string dir_path{};
				std::string descriptor_filepath{};
				std::string file_ext{ getFileExtension(nfile) };
				Resource::ResourceDescriptor descriptor;
				switch (fni->Action) {
				case FILE_ACTION_MODIFIED:
					std::wcout << L"Modified: " << filename << "\n";
				case FILE_ACTION_ADDED:
					std::wcout << L"New file: " << filename << "\n";
					if (file_ext.empty() || file_ext == ".desc") {
						break;
					}
					dir_path = getParentPath(nfile);
					descriptor = Resource::ResourceDescriptor::MakeDescriptor(nfile);
					descriptor.SaveDescriptor();
					{
						std::lock_guard lg{ m_DescriptorListMtx };
						m_Descriptors.emplace(dir_path, descriptor);
					}
					break;
				case FILE_ACTION_REMOVED:
					std::wcout << L"Removed: " << filename << "\n";
					if (file_ext == ".desc") {
						continue;
					}
					dir_path = getParentPath(nfile);
					descriptor_filepath = nfile.substr(0, nfile.find_last_of(".")) + ".desc";
					if (std::filesystem::exists(descriptor_filepath)) {
						std::filesystem::remove(descriptor_filepath);
						{
							std::lock_guard lg{ m_DescriptorListMtx };
							auto files = GetFiles(dir_path);
							for (auto it = files.first; it != files.second; ++it) {
								if (normalizePath(it->second.m_RawFileInfo.m_RawSourcePath) == nfile) {
									m_Descriptors.erase(it); // erase just this one
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
		}
	}

	CloseHandle(hDir);
}