#include "Manager/BuildManager.hpp"
#include <future>
#include <filesystem>
#include <Engine.hpp>
#include <rsc-ext/rp.hpp>

#include <windows.h>
#include <fstream>
#include <vector>
#include <iostream>

#include "Screens/EditorMain.hpp"

// Structures for parsing ICO files
#pragma pack(push, 2)
struct ICONDIR {
	WORD idReserved;   // Reserved (must be 0)
	WORD idType;       // Resource Type (1 for icons)
	WORD idCount;      // Number of images
};

struct ICONDIRENTRY {
	BYTE bWidth;       // Width of the image
	BYTE bHeight;      // Height of the image
	BYTE bColorCount;  // Number of colors (0 if >=8bpp)
	BYTE bReserved;    // Reserved
	WORD wPlanes;      // Color planes
	WORD wBitCount;    // Bits per pixel
	DWORD dwBytesInRes;// Size of the image data
	DWORD dwImageOffset;// Offset of image data in file
};
#pragma pack(pop)

// Resource structures
struct GRPICONDIRENTRY {
	BYTE bWidth;
	BYTE bHeight;
	BYTE bColorCount;
	BYTE bReserved;
	WORD wPlanes;
	WORD wBitCount;
	DWORD dwBytesInRes;
	WORD nID; // Resource ID
};

struct GRPICONDIR {
	WORD idReserved;
	WORD idType;
	WORD idCount;
	// Followed by GRPICONDIRENTRY array
};

bool InjectIcon(std::string const& exePath, std::string const& icoPath) {
	std::ifstream icoFile(icoPath, std::ios::binary);
	if (!icoFile || std::filesystem::path(icoPath).extension().string() != ".ico") {
		//std::cerr << "Failed to open icon file.\n";
		return false;
	}

	ICONDIR iconDir;
	icoFile.read(reinterpret_cast<char*>(&iconDir), sizeof(iconDir));

	std::vector<ICONDIRENTRY> entries(iconDir.idCount);
	icoFile.read(reinterpret_cast<char*>(entries.data()), iconDir.idCount * sizeof(ICONDIRENTRY));

	HANDLE hUpdate = BeginUpdateResourceA(exePath.c_str(), FALSE);
	if (!hUpdate) {
		//std::cerr << "BeginUpdateResource failed.\n";
		return false;
	}

	// Build group icon resource
	GRPICONDIR grpDir;
	grpDir.idReserved = 0;
	grpDir.idType = 1;
	grpDir.idCount = iconDir.idCount;

	std::vector<GRPICONDIRENTRY> grpEntries(iconDir.idCount);

	for (int i = 0; i < iconDir.idCount; ++i) {
		std::vector<char> image(entries[i].dwBytesInRes);
		icoFile.seekg(entries[i].dwImageOffset, std::ios::beg);
		icoFile.read(image.data(), entries[i].dwBytesInRes);

		// Assign resource ID starting at 1
		grpEntries[i].bWidth = entries[i].bWidth;
		grpEntries[i].bHeight = entries[i].bHeight;
		grpEntries[i].bColorCount = entries[i].bColorCount;
		grpEntries[i].bReserved = entries[i].bReserved;
		grpEntries[i].wPlanes = entries[i].wPlanes;
		grpEntries[i].wBitCount = entries[i].wBitCount;
		grpEntries[i].dwBytesInRes = entries[i].dwBytesInRes;
		grpEntries[i].nID = WORD(i) + 1;

		// Inject RT_ICON resource
		if (!UpdateResourceA(hUpdate, RT_ICON, MAKEINTRESOURCE(i + 1), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
			image.data(), entries[i].dwBytesInRes)) {
			//std::cerr << "UpdateResource (RT_ICON) failed.\n";
		}
	}

	// Inject RT_GROUP_ICON resource
	size_t grpSize = sizeof(GRPICONDIR) + iconDir.idCount * sizeof(GRPICONDIRENTRY);
	std::vector<char> grpData(grpSize);
	memcpy(grpData.data(), &grpDir, sizeof(GRPICONDIR));
	memcpy(grpData.data() + sizeof(GRPICONDIR), grpEntries.data(), iconDir.idCount * sizeof(GRPICONDIRENTRY));

	if (!UpdateResourceA(hUpdate, RT_GROUP_ICON, "MAINICON", MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
		grpData.data(), DWORD(grpSize))) {
		//std::cerr << "UpdateResource (RT_GROUP_ICON) failed.\n";
	}

	if (!EndUpdateResourceA(hUpdate, FALSE)) {
		//std::cerr << "EndUpdateResource failed.\n";
		return false;
	}

	//std::cout << "Icon injected successfully!\n";
	return true;
}

void MakeTemplateExecutable(std::string const& outputDir, BuildConfiguration const& config) {
	std::string const& exeName = config.output_name;
	std::string const& iconPath = std::string(Engine::getWorkingDir().data()) + "/" + config.icon_relative_path;
	const bool isFullscreen = config.windowing_mode == BuildWindowMode::fullscreen;
	std::string curr_dir = std::filesystem::current_path().string();
	std::string exePath = outputDir + "/" + exeName + ".exe";
	std::filesystem::copy_file(curr_dir + "/bin/application.exe", exePath, std::filesystem::copy_options::update_existing);
	std::filesystem::copy_file(curr_dir + "/fmod.dll", outputDir + "/fmod.dll", std::filesystem::copy_options::update_existing);
	std::filesystem::copy_file(curr_dir + "/mono-2.0-sgen.dll", outputDir + "/mono-2.0-sgen.dll", std::filesystem::copy_options::update_existing);
	std::filesystem::recursive_directory_iterator rdit{ curr_dir + "/assets/shaders"};
	for (auto const& cde : rdit) {
		if (!cde.is_directory()) {
			std::string dest = outputDir + "/" + rp::utility::get_relative_path(cde.path().string(), curr_dir);
			std::filesystem::path parent = std::filesystem::path(dest).parent_path();
			if (!std::filesystem::exists(parent)) {
				std::filesystem::create_directories(parent);
			}
			std::filesystem::copy_file(cde, dest, std::filesystem::copy_options::update_existing);
		}
	}
	InjectIcon(exePath, iconPath);
	std::string config_output = std::string(outputDir + "/config.yaml");
	Engine::GenerateDefaultConfig(config_output);
	YAML::Node nd = YAML::LoadFile(config_output);
	nd["window"]["title"] = exeName;
	nd["window"]["fullscreen"] = isFullscreen;
	nd["window"]["width"] = config.window_size.width;
	nd["window"]["height"] = config.window_size.height;
	std::ofstream ofs(config_output);
	ofs << nd;
	ofs.close();
}

std::vector<std::string> GetSceneManifestSceneList(std::string const& manifestPath) {
	std::ifstream ifs(manifestPath, std::ios::binary);
	unsigned scene_ct{};
	std::vector<std::string> scenes;
	if (ifs) {
		ifs.read(reinterpret_cast<char*>(&scene_ct), sizeof(unsigned));
		scenes.reserve(scene_ct);

		while (scene_ct--) {
			int index;
			ifs.read(reinterpret_cast<char*>(&index), sizeof(int));
			std::string scene_path;
			// read until null terminator
			std::getline(ifs, scene_path, '\0');
			scenes.emplace_back(scene_path);
		}
	}
	ifs.close();
	return scenes;
}

void FindValues(const YAML::Node& node, const std::string& targetKey, std::vector<YAML::Node>& results) {
	if (!node.IsMap()) return;
	for (auto it : node) {
		if (it.first.as<std::string>() == targetKey) {
			results.push_back(it.second);
		}
		FindValues(it.second, targetKey, results);
	}
}

void FindNodesWithKeys(const YAML::Node& node,const std::vector<std::string>& targetKeys, std::vector<YAML::Node>& results) {
	if (!node.IsDefined() || !node || node.IsNull() || targetKeys.empty()) return;
	if (node.IsMap()) {
		bool is_match{ true };
		for (std::string const& key : targetKeys) {
			if (!node[key].IsDefined()) {
				is_match = false;
				break;
			}
		}
		if (is_match) {
			results.push_back(node);
		}
		for (auto it : node) {
			FindNodesWithKeys(it.second, targetKeys, results);
		}
	}
	else if (node.IsSequence()) {
		for (auto it : node) {
			FindNodesWithKeys(it, targetKeys, results);
		}
	}
}

std::pair<std::uint64_t, std::uint32_t> DiscoverMonoRuntimeFiles() {
	std::string curr_dir = std::filesystem::current_path().string();
	std::uint64_t total_mono_bytes{};
	std::uint32_t total_mono_file_ct{};
	std::filesystem::recursive_directory_iterator rdit{ curr_dir + "/lib/mono" };
	for (auto const& cde : rdit) {
		if (!cde.is_directory()) {
			total_mono_bytes += cde.file_size();
			total_mono_file_ct++;
		}
	}
	return { total_mono_bytes, total_mono_file_ct };
}

std::unordered_set<rp::BasicIndexedGuid> /*BuildManager::*/DiscoverSceneResources(std::uint64_t* total_sz, std::uint32_t* file_ct, bool discover_soft_dependencies) {
	std::string proj_dir = std::string(Engine::getWorkingDir().data()); //this is set to project_dir/asset
	std::string manifest_path = proj_dir + "/scene_manifest.order";
	std::unordered_set<rp::BasicIndexedGuid> res;
	auto AddResource{ [total_sz, file_ct, &res](rp::BasicIndexedGuid big) {
		if (!res.contains(big)) {
			std::string fileph = rp::utility::output_path() + "/" + big.m_guid.to_hex() + rp::ResourceTypeImporterRegistry::GetResourceExt(big.m_typeindex);
			if (std::filesystem::exists(fileph)) {
				res.insert(big);
				if (total_sz) {
					(*total_sz) += std::filesystem::directory_entry(fileph).file_size();
				}
				if (file_ct) {
					(*file_ct)++;
				}
			}
		} } };

	if (std::filesystem::exists(manifest_path)) {
		auto scene_list = GetSceneManifestSceneList(manifest_path);
		for (auto const& scene_name : scene_list) {
			std::string scene_path = proj_dir + "/" + scene_name;
			if (std::filesystem::exists(scene_path)) {
				YAML::Node scn_root = YAML::LoadFile(scene_path);
				std::vector<YAML::Node> guid_nodes;
				FindNodesWithKeys(scn_root, { "guid", "type" }, guid_nodes);
				
				for (YAML::Node guid_nd : guid_nodes) {
					rp::BasicIndexedGuid full_guid{ rp::Guid::to_guid(guid_nd["guid"].as<std::string>()), guid_nd["type"].as<std::uint64_t>()};
					if (!full_guid.m_guid)
						continue;
					if (full_guid.m_typeindex == rp::utility::string_hash("mesh")) {
						rp::BasicIndexedGuid meshmeta_guid{full_guid.m_guid, rp::utility::string_hash("meshmeta")};
						meshmeta_guid.m_guid.m_low += 1;
						AddResource(meshmeta_guid);
					}
					else if (discover_soft_dependencies) {
						switch (full_guid.m_typeindex) {
						case rp::utility::string_hash("material"):
							break;
						case rp::utility::string_hash("audio"):
							break;
						}
					}
					AddResource(full_guid);
				}
			}
		}
	}
	return res;
}

std::pair<std::uint64_t, std::uint32_t> CopyMonoRuntime(std::string const& outputDir, std::shared_ptr<BuildContext> context, std::uint64_t total_size) {
	std::string curr_dir = std::filesystem::current_path().string();
	std::uint64_t bytes_copied_mul100{};
	std::uint32_t file_copied{};
	std::filesystem::recursive_directory_iterator rdit{ curr_dir + "/lib/mono" };
	int local_progress{};
	std::string mono_base = curr_dir + "/lib";
	for (auto const& cde : rdit) {
		if (context->m_state == BuildState::ABORTED)
			break;
		if (!cde.is_directory()) {
			std::string dest = outputDir + "/" + rp::utility::get_relative_path(cde.path().string(), mono_base);
			std::filesystem::path parent = std::filesystem::path(dest).parent_path();
			if (!std::filesystem::exists(parent)) {
				std::filesystem::create_directories(parent);
			}
			std::filesystem::copy_file(cde, dest, std::filesystem::copy_options::update_existing);
			bytes_copied_mul100 += cde.file_size()*100;
			file_copied++;
			int current_progress = int(bytes_copied_mul100 / total_size);
			if (current_progress > local_progress) {
				context->m_progress100 = local_progress = current_progress;
			}
		}
	}
	return { bytes_copied_mul100, file_copied };
}

void CopySceneManifestData(std::string const& outputDir) {
	std::string proj_dir = std::string(Engine::getWorkingDir().data()); //this is set to project_dir/asset
	std::string manifest_path = outputDir + "/scene_manifest.order";
	if (std::filesystem::exists(proj_dir + "/scene_manifest.order")) {
		std::filesystem::copy_file(proj_dir + "/scene_manifest.order", manifest_path, std::filesystem::copy_options::overwrite_existing);
		auto scene_list = GetSceneManifestSceneList(manifest_path);
		for (auto const& scene_name : scene_list) {
			std::string output_scene = outputDir + "/" + scene_name;
			if (!std::filesystem::exists(std::filesystem::path(output_scene).parent_path())) {
				std::filesystem::create_directories(std::filesystem::path(output_scene).parent_path());
			}
			std::filesystem::copy_file(proj_dir + "/" + scene_name, output_scene, std::filesystem::copy_options::overwrite_existing);
		}
	}
}

std::map<std::string, rp::BasicIndexedGuid> PackageResources(std::unordered_set<rp::BasicIndexedGuid> resources, std::uint64_t total_bytes, std::uint64_t& bytes_copied_mul100, std::uint32_t& file_copied, std::string const& outputDir, std::shared_ptr<BuildContext> context) {
	std::string res_folder{ rp::utility::output_path() };
	std::map<std::string, rp::BasicIndexedGuid> pkgs;
	int local_progress{context->m_progress100};
	if (!std::filesystem::exists(outputDir)) {
		std::filesystem::create_directories(outputDir);
	}
	for (rp::BasicIndexedGuid const& rsc : resources) {
		std::string suffix = rp::ResourceTypeImporterRegistry::GetResourceExt(rsc.m_typeindex);
		if (suffix.empty())
			throw std::runtime_error("build package error: suffix not registered!");
		std::string tgtpath = res_folder + "/" + rsc.m_guid.to_hex() + suffix;
		std::string outpath = outputDir + "/" + rsc.m_guid.to_hex() + suffix;
		std::filesystem::copy_file(tgtpath, outpath, std::filesystem::copy_options::overwrite_existing);
		pkgs.emplace(outpath, rsc);
		bytes_copied_mul100 += std::filesystem::directory_entry(tgtpath).file_size() * 100;
		file_copied++;
		int current_progress = int(bytes_copied_mul100 / total_bytes);
		if (current_progress > local_progress) {
			context->m_progress100 = local_progress = current_progress;
		}
	}
	return pkgs;
}

std::future<void> BuildManager::BuildAsync(BuildConfiguration config, std::shared_ptr<BuildContext> context)
{
	context->m_progress100 = 0;
	context->m_state = BuildState::QUEUED;
	auto fut = std::async(std::launch::async, [context, config] {
		context->m_state = BuildState::IN_PROGRESS;
		try {
			std::string outputDir = config.output_dir + "/" + config.output_name;
			std::filesystem::create_directories(outputDir);
			std::uint64_t total_bytes{};
			int local_progress{};
			std::uint32_t file_ct{};
			std::unordered_set<rp::BasicIndexedGuid> rsc;
			if (!std::filesystem::exists(rp::utility::working_path() + "/audio")) {
				std::filesystem::create_directories(rp::utility::working_path() + "/audio");
			}
			if (config.resource_cleanup == ResourceCleanUpMode::none) {
				for (const auto& cde : std::filesystem::recursive_directory_iterator{ rp::utility::output_path() }) {
					if (!cde.is_directory()) {
						total_bytes += cde.file_size();
						file_ct++;
					}
				}
			}
			else {
				rsc = DiscoverSceneResources(&total_bytes, &file_ct, false);
				if (config.resource_cleanup == ResourceCleanUpMode::minimal) {
					for (const auto& cde : std::filesystem::recursive_directory_iterator{ rp::utility::output_path() }) {
						if (!cde.is_directory() && cde.path().extension() == ".texture") {
							rp::BasicIndexedGuid tex_guid{ rp::Guid::to_guid(cde.path().stem().string()), rp::utility::string_hash("texture") };
							if (!rsc.contains(tex_guid)) {
								rsc.insert(tex_guid);
								total_bytes += cde.file_size();
								file_ct++;
							}
						}
					}
				}
			}
			for (const auto& cde : std::filesystem::recursive_directory_iterator{ rp::utility::working_path() + "/audio" }) {
				if (!cde.is_directory()) {
					total_bytes += cde.file_size();
					file_ct++;
				}
			}
			auto [mono_bytes, mono_files] = DiscoverMonoRuntimeFiles();
			total_bytes += mono_bytes;
			file_ct += mono_files;

			std::uint64_t bytes_copied_mul100{};
			std::uint32_t file_copied{};

			if (context->m_state != BuildState::ABORTED) {
				MakeTemplateExecutable(outputDir, config);
				CopySceneManifestData(outputDir);
				auto [mono_bytes_copied_mul100, mono_files_copied] = CopyMonoRuntime(outputDir, context, total_bytes);
				bytes_copied_mul100 = mono_bytes_copied_mul100;
				file_copied = mono_files_copied;
				//copy game binaries
				std::string precompiledpath = outputDir + "/data/managed";
				if (!std::filesystem::exists(precompiledpath)) {
					std::filesystem::create_directories(precompiledpath);
				}
				std::string curr = Engine::getWorkingDir().data();
				std::filesystem::copy_file(curr + "/managed/GameAssembly.dll", precompiledpath+"/GameAssembly.dll", std::filesystem::copy_options::overwrite_existing);
				std::filesystem::copy_file(std::filesystem::current_path().string() + "/bin/BasilEngine.dll", precompiledpath + "/BasilEngine.dll", std::filesystem::copy_options::overwrite_existing);
				std::filesystem::copy_file(std::filesystem::current_path().string() + "/bin/Engine.Bindings.dll", precompiledpath + "/Engine.Bindings.dll", std::filesystem::copy_options::overwrite_existing);
			}
			if (config.resource_cleanup == ResourceCleanUpMode::none) {
				for (const auto& cde : std::filesystem::recursive_directory_iterator{ rp::utility::output_path() }) {
					if (context->m_state == BuildState::ABORTED)
						return;
					if (!cde.is_directory()) {
						std::string dest = outputDir + "/assets/bin/" + rp::utility::get_relative_path(cde.path().string(), rp::utility::output_path());
						std::filesystem::path parent = std::filesystem::path(dest).parent_path();
						if (!std::filesystem::exists(parent)) {
							std::filesystem::create_directories(parent);
						}
						std::filesystem::copy_file(cde, dest, std::filesystem::copy_options::overwrite_existing);
						bytes_copied_mul100 += cde.file_size() * 100;
						file_copied++;
						int current_progress = int(bytes_copied_mul100 / total_bytes);
						if (current_progress > local_progress) {
							context->m_progress100 = local_progress = current_progress;
						}
					}
				}
				std::string assetlist_loc = outputDir + "/assets/bin/.assetlist";
				if (std::filesystem::exists(assetlist_loc)) {
					std::filesystem::rename(assetlist_loc, outputDir + "/resource.manifest");
				}
			}
			else {
				std::map<std::string, rp::BasicIndexedGuid> packages = PackageResources(rsc, total_bytes, bytes_copied_mul100, file_copied, outputDir + "/assets/bin/", context);
				rp::serialization::yaml_serializer::serialize(packages, outputDir + "/resource.manifest");
			}
			for (const auto& cde : std::filesystem::recursive_directory_iterator{ rp::utility::working_path() + "/audio" }) {
				if (context->m_state == BuildState::ABORTED)
					return;
				if (!cde.is_directory() && cde.path().extension().string() != ".desc") {
					std::string dest = outputDir + "/assets/" + rp::utility::get_relative_path(cde.path().string(), rp::utility::working_path());
					std::filesystem::path parent = std::filesystem::path(dest).parent_path();
					if (!std::filesystem::exists(parent)) {
						std::filesystem::create_directories(parent);
					}
					std::filesystem::copy_file(cde, dest, std::filesystem::copy_options::overwrite_existing);
					bytes_copied_mul100 += cde.file_size()*100;
					file_copied++; 
					int current_progress = int(bytes_copied_mul100 / total_bytes);
					if (current_progress > local_progress) {
						context->m_progress100 = local_progress = current_progress;
					}
				}
			}
			context->m_state = BuildState::SUCCESS;
			context->m_progress100 = 100;
		}
		catch (...) {
			context->m_state = BuildState::FAILED;
		}
		});
	return fut;
}

BuildConfiguration BuildManager::LoadBuildConfiguration()
{
	std::string configname = std::string(Engine::getWorkingDir().data()) + "/build.yaml";
	return (std::filesystem::exists(configname)) ? rp::serialization::yaml_serializer::deserialize<BuildConfiguration>(configname) : BuildConfiguration{};
}

void BuildManager::SaveBuildConfiguration(BuildConfiguration const& config)
{
	std::string configname = std::string(Engine::getWorkingDir().data()) + "/build.yaml";
	rp::serialization::yaml_serializer::serialize(config, configname);
}
