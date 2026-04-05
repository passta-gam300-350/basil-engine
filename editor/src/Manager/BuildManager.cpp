#include "Manager/BuildManager.hpp"
#include <future>
#include <filesystem>
#include <Engine.hpp>
#include <rsc-ext/rp.hpp>

#include <windows.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <algorithm>

#include <Manager/ResourceSystem.hpp>

#include "Screens/EditorMain.hpp"
#include <descriptors/descriptors.hpp>
#include <importer/importer.hpp>
#include <Manager/MonoEntityManager.hpp>
#include <MonoManager.hpp>
#include <ScriptCompiler.hpp>

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

void MakeTemplateExecutable(std::string const& outputDir, BuildConfiguration const& config, std::string const& projectDir) {
	std::string const& exeName = config.output_name;
	std::string iconPath = projectDir + "/" + config.icon_relative_path;
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
	if (!std::filesystem::exists(iconPath)) {
		iconPath = projectDir + "/Icon.ico"; //default to this
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

DescriptorIndex BuildManager::BuildDescriptorIndex(std::string const& assetsDir) {
	DescriptorIndex index;
	if (!std::filesystem::exists(assetsDir))
		return index;
	for (auto const& entry : std::filesystem::recursive_directory_iterator(assetsDir)) {
		if (!entry.is_directory() && entry.path().extension() == ".desc") {
			std::string descPath = entry.path().string();
			try {
				rp::BasicIndexedGuid big = rp::ResourceTypeImporterRegistry::GetDescriptorGuid(descPath);
				if (big.m_guid) {
					index.emplace(big.m_guid, DescriptorInfo{ descPath, rp::ResourceTypeImporterRegistry::GetDescriptorImporterType(descPath)});
				}
			}
			catch (...) {}
		}
	}
	return index;
}

std::unordered_set<rp::BasicIndexedGuid> DiscoverSceneResourcesWithIndex(std::string const& projectDir, DescriptorIndex const* descIndex, std::uint64_t* total_sz, std::uint32_t* file_ct, std::unordered_set<std::string>* audio_paths = nullptr, BuildContext* context=nullptr) {
	std::string manifest_path = projectDir + "/scene_manifest.order";
	std::unordered_set<rp::BasicIndexedGuid> res;
	auto AddResource{ [&](rp::BasicIndexedGuid big) {
		if (!res.contains(big)) {
			std::string suffix = rp::ResourceTypeImporterRegistry::GetResourceExt(big.m_typeindex);
			std::string fileph = rp::utility::output_path() + "/" + big.m_guid.to_hex() + suffix;
			if (std::filesystem::exists(fileph)) {
				res.insert(big);
				if (total_sz) {
					(*total_sz) += std::filesystem::directory_entry(fileph).file_size();
				}
				if (file_ct) {
					(*file_ct)++;
				}
			}
			else if (descIndex) {
				auto it = descIndex->find(big.m_guid);
				if (it != descIndex->end()) {
					std::cout << "  Importing missing resource: " << big.m_guid.to_hex() << suffix << "\n";
					rp::ResourceTypeImporterRegistry::Import(it->second.importer_type, it->second.desc_path, fileph);
					if (std::filesystem::exists(fileph)) {
						res.insert(big);
						if (total_sz) {
							(*total_sz) += std::filesystem::directory_entry(fileph).file_size();
						}
						if (file_ct) {
							(*file_ct)++;
						}
					}
				}
				else {
					std::cerr << "  Warning: resource " << big.m_guid.to_hex() << suffix << " not found in .imports and no descriptor exists." << std::endl;
				}
			}
		}
	} };

	if (std::filesystem::exists(manifest_path)) {
		auto scene_list = GetSceneManifestSceneList(manifest_path);
		if (context) {
			context->m_scenes_total = static_cast<std::uint32_t>(scene_list.size());
		}
		for (auto const& scene_name : scene_list) {
			std::string scene_path = projectDir + "/" + scene_name;
			if (std::filesystem::exists(scene_path)) {
				YAML::Node scn_root = YAML::LoadFile(scene_path);
				std::vector<YAML::Node> guid_nodes;
				std::vector<YAML::Node> scn_render_nodes;
				FindValues(scn_root, { "face_textures" }, scn_render_nodes);
				FindNodesWithKeys(scn_root, { "guid", "type" }, guid_nodes);

				for (YAML::Node scn_render_nd : scn_render_nodes) {
					for (auto val : scn_render_nd) {
						rp::BasicIndexedGuid full_guid{ rp::Guid::to_guid(val.as<std::string>()), rp::utility::string_hash("texture")};
						AddResource(full_guid);
					}
				}

				for (YAML::Node guid_nd : guid_nodes) {
					rp::BasicIndexedGuid full_guid{ rp::Guid::to_guid(guid_nd["guid"].as<std::string>()), guid_nd["type"].as<std::uint64_t>() };
					if (!full_guid.m_guid)
						continue;
					if (full_guid.m_typeindex == rp::utility::string_hash("mesh")) {
						rp::BasicIndexedGuid meshmeta_guid{ full_guid.m_guid, rp::utility::string_hash("meshmeta") };
						meshmeta_guid.m_guid.m_low += 1;
						AddResource(meshmeta_guid);
					}
					else if (full_guid.m_typeindex == rp::utility::string_hash("material") || full_guid.m_typeindex == rp::utility::string_hash("audio")) {
						auto descIt = descIndex ? descIndex->find(full_guid.m_guid) : DescriptorIndex::const_iterator{};
						std::string descPath;
						if (descIndex && descIt != descIndex->end()) {
							descPath = descIt->second.desc_path;
						}
						else {
							std::string suffix = rp::ResourceTypeImporterRegistry::GetResourceExt(full_guid.m_typeindex);
							descPath = rp::utility::working_path() + "/" + full_guid.m_guid.to_hex() + suffix + ".desc";
						}
						if (std::filesystem::exists(descPath)) {
							try {
								MaterialDescriptor matDesc;
								AudioDescriptor audDesc;
								switch (full_guid.m_typeindex) {
								case rp::utility::string_hash("material"):
									matDesc = rp::serialization::yaml_serializer::deserialize<MaterialDescriptor>(descPath);
									for (auto const& [key, texGuid] : matDesc.material.texture_properties) {
										if (texGuid) {
											AddResource({ texGuid, rp::utility::string_hash("texture") });
										}
									}
									break;
								case rp::utility::string_hash("audio"):
									if (audio_paths) {
										audDesc = rp::serialization::yaml_serializer::deserialize<AudioDescriptor>(descPath);
										audio_paths->insert(audDesc.base.m_source);
										*total_sz+=std::filesystem::directory_entry(rp::utility::working_path()+audDesc.base.m_source).file_size();
									}
									break;
								default:
									break;
								}
							}
							catch (...) {}
						}
						else if (std::string fileph = rp::utility::output_path() + "/" + full_guid.m_guid.to_hex() + ".audio";
								full_guid.m_typeindex == rp::utility::string_hash("audio") && std::filesystem::exists(fileph)) {
							std::cout << "  Warning: audio descriptor not found for " << full_guid.m_guid.to_hex() << " but audio import exists, likely a deprecated guid, fallback to extracting audio dependencies from imported audio\n";
							AudioResourceData audioData = rp::serialization::serializer<"bin">::deserialize<AudioResourceData>(
								fileph
							);
							std::string aph = audioData.sourcePath;
							std::uint64_t asset_loc = aph.find("assets");
							std::string raph = (asset_loc != std::string::npos) ? aph.substr(asset_loc + 7) : aph;

							aph = rp::utility::working_path() + "/" + raph;
							try {
								if (std::filesystem::exists(aph)) {
									audio_paths->insert(raph);
									*total_sz += std::filesystem::directory_entry(std::filesystem::path(aph).lexically_normal()).file_size();
								}
								else {
									std::cout << "  Warning: audio source not found for " << aph << "\n";
								}
							}
							catch (std::exception const& e){
								std::cout << "   Error: " << e.what() << "\n";
							}
						}
					}
					AddResource(full_guid);
				}
			}
			if (context) {
				context->m_scenes_discovered.fetch_add(1, std::memory_order_relaxed);
			}
		}
	}
	return res;
}

[[deprecated]] std::unordered_set<rp::BasicIndexedGuid> DiscoverSceneResources(std::uint64_t* total_sz, std::uint32_t* file_ct, bool discover_soft_dependencies) {
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
					else if (discover_soft_dependencies && full_guid.m_typeindex == rp::utility::string_hash("material")) {
						std::string suffix = rp::ResourceTypeImporterRegistry::GetResourceExt(full_guid.m_typeindex);
						std::string descPath = rp::utility::working_path() + "/" + full_guid.m_guid.to_hex() + suffix + ".desc";
						if (!std::filesystem::exists(descPath)) {
							descPath = rp::utility::working_path() + "/" + full_guid.m_guid.to_hex() + ".desc";
						}
						if (std::filesystem::exists(descPath)) {
							try {
								MaterialDescriptor matDesc = rp::serialization::yaml_serializer::deserialize<MaterialDescriptor>(descPath);
								for (auto const& [key, texGuid] : matDesc.material.texture_properties) {
									if (texGuid) {
										AddResource({ texGuid, rp::utility::string_hash("texture") });
									}
								}
							}
							catch (...) {}
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

void CopySceneManifestData(std::string const& outputDir, std::string const& projectDir) {
	std::string manifest_path = outputDir + "/scene_manifest.order";
	if (std::filesystem::exists(projectDir + "/scene_manifest.order")) {
		std::filesystem::copy_file(projectDir + "/scene_manifest.order", manifest_path, std::filesystem::copy_options::overwrite_existing);
		auto scene_list = GetSceneManifestSceneList(manifest_path);
		for (auto const& scene_name : scene_list) {
			std::string output_scene = outputDir + "/" + scene_name;
			if (!std::filesystem::exists(std::filesystem::path(output_scene).parent_path())) {
				std::filesystem::create_directories(std::filesystem::path(output_scene).parent_path());
			}
			std::filesystem::copy_file(projectDir + "/" + scene_name, output_scene, std::filesystem::copy_options::overwrite_existing);
		}
	}
}

std::map<std::string, rp::BasicIndexedGuid> PackageResources(std::unordered_set<rp::BasicIndexedGuid> resources, std::string const& outputDir, std::uint64_t total_bytes = 0, std::uint64_t* bytes_copied_mul100 = nullptr, std::uint32_t* file_copied = nullptr, std::shared_ptr<BuildContext> context = nullptr) {
	std::string res_folder{ rp::utility::output_path() };
	std::map<std::string, rp::BasicIndexedGuid> pkgs;
	int local_progress{ context ? context->m_progress100.load() : 0};
	uint64_t local_bytes_cpd_mul100 = bytes_copied_mul100 ? *bytes_copied_mul100 : 0;
	std::uint32_t fcp = file_copied ? *file_copied : 0;
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
		local_bytes_cpd_mul100 += std::filesystem::directory_entry(tgtpath).file_size() * 100;
		fcp++;
		if (context)
			context->m_files_copied++;
		if (total_bytes && context) {
			int current_progress = int(local_bytes_cpd_mul100 / total_bytes);
			if (current_progress > local_progress) {
				context->m_progress100 = local_progress = current_progress;
			}
		}
	}
	if (bytes_copied_mul100)
		*bytes_copied_mul100 = local_bytes_cpd_mul100;
	if (file_copied)
		*file_copied = fcp;
;	return pkgs;
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
			context->m_output_path = outputDir;
			std::uint64_t total_bytes{};
			int local_progress{};
			std::uint32_t file_ct{};
			std::unordered_set<rp::BasicIndexedGuid> rsc;
			std::unordered_set<std::string> audios;
			DescriptorIndex desc_idx;

			context->m_phase = BuildPhase::DiscoveringResources;
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
				for (const auto& cde : std::filesystem::recursive_directory_iterator{ rp::utility::working_path() + "/audio" }) {
					if (!cde.is_directory()) {
						total_bytes += cde.file_size();
						file_ct++;
					}
				}
			}
			else {
				desc_idx = BuildDescriptorIndex(rp::utility::working_path());
				rsc = DiscoverSceneResourcesWithIndex(std::string(Engine::getWorkingDir().data()), &desc_idx, &total_bytes, &file_ct, &audios, context.get());
				context->m_resources_found = static_cast<std::uint32_t>(rsc.size());
				file_ct += audios.size();
			}
			auto [mono_bytes, mono_files] = DiscoverMonoRuntimeFiles();
			total_bytes += mono_bytes;
			file_ct += mono_files;
			context->m_files_total = file_ct;

			std::uint64_t bytes_copied_mul100{};
			std::uint32_t file_copied{};

			if (context->m_state != BuildState::ABORTED) {
				context->m_phase = BuildPhase::CreatingExecutable;
				std::string projDir = std::string(Engine::getWorkingDir().data());
				MakeTemplateExecutable(outputDir, config, projDir);
				CopySceneManifestData(outputDir, projDir);

				context->m_phase = BuildPhase::CopyingMonoRuntime;
				auto [mono_bytes_copied_mul100, mono_files_copied] = CopyMonoRuntime(outputDir, context, total_bytes);
				bytes_copied_mul100 = mono_bytes_copied_mul100;
				file_copied = mono_files_copied;

				context->m_phase = BuildPhase::CopyingManagedDLLs;
				std::string precompiledpath = outputDir + "/data/managed";
				if (!std::filesystem::exists(precompiledpath)) {
					std::filesystem::create_directories(precompiledpath);
				}
				std::filesystem::copy_file(projDir + "/managed/GameAssembly.dll", precompiledpath+"/GameAssembly.dll", std::filesystem::copy_options::overwrite_existing);
				std::filesystem::copy_file(std::filesystem::current_path().string() + "/bin/BasilEngine.dll", precompiledpath + "/BasilEngine.dll", std::filesystem::copy_options::overwrite_existing);
				std::filesystem::copy_file(std::filesystem::current_path().string() + "/bin/Engine.Bindings.dll", precompiledpath + "/Engine.Bindings.dll", std::filesystem::copy_options::overwrite_existing);
			}
			context->m_phase = BuildPhase::PackagingResources;
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
						context->m_files_copied = file_copied;
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
				std::map<std::string, rp::BasicIndexedGuid> packages = PackageResources(rsc, outputDir + "/assets/bin/", total_bytes, &bytes_copied_mul100, &file_copied, context);
				context->m_files_copied = file_copied;
				rp::serialization::yaml_serializer::serialize(packages, outputDir + "/resource.manifest");
			}
			context->m_phase = BuildPhase::CopyingAudio;
			if (config.resource_cleanup == ResourceCleanUpMode::none) {
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
						bytes_copied_mul100 += cde.file_size() * 100;
						file_copied++;
						context->m_files_copied = file_copied;
						int current_progress = int(bytes_copied_mul100 / total_bytes);
						if (current_progress > local_progress) {
							context->m_progress100 = local_progress = current_progress;
						}
					}
				}
			}
			else {
				for (std::string const& audio : audios) {
					if (context->m_state == BuildState::ABORTED)
						return;
					std::string const audio_path = rp::utility::working_path()+ "/" + audio;
					if (std::filesystem::exists(audio_path)) {
						std::string dest = outputDir + "/assets/" + audio;
						std::filesystem::path parent = std::filesystem::path(dest).parent_path();
						if (!std::filesystem::exists(parent)) {
							std::filesystem::create_directories(parent);
						}
						std::filesystem::copy_file(audio_path, dest, std::filesystem::copy_options::overwrite_existing);
						bytes_copied_mul100 += std::filesystem::directory_entry(audio_path).file_size() * 100;
						file_copied++;
						context->m_files_copied = file_copied;
						int current_progress = int(bytes_copied_mul100 / total_bytes);
						if (current_progress > local_progress) {
							context->m_progress100 = local_progress = current_progress;
						}
					}
				}
			}
			context->m_phase = BuildPhase::Done;
			context->m_state = BuildState::SUCCESS;
			context->m_progress100 = 100;
			context->m_files_copied = file_copied;
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

BuildConfiguration BuildManager::LoadBuildConfigurationFrom(std::string const& projectDir)
{
	std::string configname = projectDir + "/build.yaml";
	return (std::filesystem::exists(configname)) ? rp::serialization::yaml_serializer::deserialize<BuildConfiguration>(configname) : BuildConfiguration{};
}

void BuildManager::SaveBuildConfiguration(BuildConfiguration const& config)
{
	std::string configname = std::string(Engine::getWorkingDir().data()) + "/build.yaml";
	rp::serialization::yaml_serializer::serialize(config, configname);
}

static void CopyMonoRuntimeSync(std::string const& outputDir) {
	std::string curr_dir = std::filesystem::current_path().string();
	std::filesystem::recursive_directory_iterator rdit{ curr_dir + "/lib/mono" };
	std::string mono_base = curr_dir + "/lib";
	for (auto const& cde : rdit) {
		if (!cde.is_directory()) {
			std::string dest = outputDir + "/" + rp::utility::get_relative_path(cde.path().string(), mono_base);
			std::filesystem::path parent = std::filesystem::path(dest).parent_path();
			if (!std::filesystem::exists(parent)) {
				std::filesystem::create_directories(parent);
			}
			std::filesystem::copy_file(cde, dest, std::filesystem::copy_options::update_existing);
		}
	}
}

int BuildManager::BuildSync(BuildConfiguration config, std::string projectDir, std::string outputDir)
{
	try {
		std::string assetsDir = projectDir + "/assets";
		std::string importsDir = projectDir + "/.imports";
		std::string manifestPath = projectDir + "/scene_manifest.order";

		if (!std::filesystem::exists(manifestPath)) {
			std::cerr << "error: scene_manifest.order not found at " << manifestPath << std::endl;
			return 1;
		}

		rp::utility::working_path() = assetsDir;
		rp::utility::output_path() = importsDir;

		std::string fullOutputDir = outputDir + "/" + config.output_name;
		std::filesystem::create_directories(fullOutputDir);

		std::cout << "[1/6] Compiling scripts..." << std::endl;
		MonoEntityManager::GetInstance().initialize();
		std::string scriptsDir = assetsDir + "/scripts";
		if (std::filesystem::exists(scriptsDir)) {
			MonoEntityManager::GetInstance().AddSearchDirectory(scriptsDir.c_str());
		}
		std::string managedOutput = fullOutputDir + "/data/managed";
		std::filesystem::create_directories(managedOutput);
		MonoEntityManager::GetInstance().SetOutputDirectory(managedOutput.c_str());

		MonoManager::disableCompile(false);
		MonoEntityManager::GetInstance().StartCompilation();

		ScriptCompiler* compiler = MonoManager::GetCompiler();
		if (compiler && (!compiler->LastCompileSucceeded() || !compiler->diagnostics.empty())) {
			bool hasErrors = false;
			for (auto const& diag : compiler->diagnostics) {
				std::string severity = diag.severity;
				std::transform(severity.begin(), severity.end(), severity.begin(),
					[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				std::cerr << diag.filename << "(" << diag.position << "): "
					<< diag.severity << " " << diag.diagnosticID << ": " << diag.message << std::endl;
				if (severity == "error") hasErrors = true;
			}
			if (hasErrors || !compiler->LastCompileSucceeded()) {
				std::cerr << "error: script compilation failed." << std::endl;
				return 1;
			}
		}

		std::cout << "[2/6] Discovering resources..." << std::endl;
		if (!std::filesystem::exists(importsDir)) {
			std::filesystem::create_directories(importsDir);
		}
		DescriptorIndex descIndex = BuildDescriptorIndex(assetsDir);
		std::uint64_t total_bytes{};
		std::uint32_t file_ct{};
		std::unordered_set<std::string> audios;
		std::unordered_set<rp::BasicIndexedGuid> rsc = DiscoverSceneResourcesWithIndex(projectDir, &descIndex, &total_bytes, &file_ct, &audios);
		if (total_bytes == 0 && file_ct == 0) {
			for (const auto& cde : std::filesystem::recursive_directory_iterator{ importsDir }) {
				if (!cde.is_directory()) {
					total_bytes += cde.file_size();
					file_ct++;
				}
			}
		}

		std::cout << "[3/6] Creating executable..." << std::endl;
		MakeTemplateExecutable(fullOutputDir, config, projectDir);

		std::cout << "[4/6] Copying Mono runtime..." << std::endl;
		CopyMonoRuntimeSync(fullOutputDir);

		std::cout << "[5/6] Packaging resources..." << std::endl;
		CopySceneManifestData(fullOutputDir, projectDir);

		std::filesystem::copy_file(
			std::filesystem::current_path().string() + "/bin/BasilEngine.dll",
			managedOutput + "/BasilEngine.dll",
			std::filesystem::copy_options::overwrite_existing);
		std::filesystem::copy_file(
			std::filesystem::current_path().string() + "/bin/Engine.Bindings.dll",
			managedOutput + "/Engine.Bindings.dll",
			std::filesystem::copy_options::overwrite_existing);

		std::string outputAssetDir = fullOutputDir + "/assets/bin";

		if (!std::filesystem::exists(outputAssetDir)) {
			std::filesystem::create_directories(outputAssetDir);
		}
		auto packages = PackageResources(rsc, outputAssetDir);
		for (std::string const& audio : audios) {
			std::string const audio_path = rp::utility::working_path() + "/" + audio;
			std::cout << "   Packing audio " << audio_path << "\n";
			if (std::filesystem::exists(audio_path)) {
				std::string dest = fullOutputDir + "/assets/" + audio;
				std::filesystem::path parent = std::filesystem::path(dest).parent_path();
				if (!std::filesystem::exists(parent)) {
					std::filesystem::create_directories(parent);
				}
				std::filesystem::copy_file(audio_path, dest, std::filesystem::copy_options::overwrite_existing);
			}
		}
		rp::serialization::yaml_serializer::serialize(packages, fullOutputDir + "/resource.manifest");

		std::cout << "[6/6] Done!" << std::endl;
		std::cout << "Build output: " << fullOutputDir << std::endl;
		return 0;
	}
	catch (std::exception const& e) {
		std::cerr << "error: build failed with exception: " << e.what() << std::endl;
		return 1;
	}
	catch (...) {
		std::cerr << "error: build failed with unknown exception." << std::endl;
		return 1;
	}
}
