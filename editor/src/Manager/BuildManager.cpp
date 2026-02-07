#include "Manager/BuildManager.hpp"
#include <future>
#include <filesystem>
#include <Engine.hpp>
#include <rsc-ext/rp.hpp>

void MakeTemplateExecutable(std::string const& outputDir, std::string const& exeName, bool isFullscreen) {
	std::string curr_dir = std::filesystem::current_path().string();
	std::filesystem::copy_file(curr_dir + "/bin/application.exe", outputDir + "/" + exeName+".exe", std::filesystem::copy_options::update_existing);
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
	std::string config_output = std::string(outputDir + "/config.yaml");
	Engine::GenerateDefaultConfig(config_output);
	YAML::Node nd = YAML::LoadFile(config_output);
	nd["window"]["title"] = exeName;
	nd["window"]["fullscreen"] = isFullscreen;
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
			int current_progress = bytes_copied_mul100 / total_size;
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
		std::filesystem::copy_file(proj_dir + "/scene_manifest.order", manifest_path, std::filesystem::copy_options::update_existing);
		auto scene_list = GetSceneManifestSceneList(manifest_path);
		for (auto const& scene_name : scene_list) {
			std::string output_scene = outputDir + "/" + scene_name;
			if (!std::filesystem::exists(std::filesystem::path(output_scene).parent_path())) {
				std::filesystem::create_directories(std::filesystem::path(output_scene).parent_path());
			}
			std::filesystem::copy_file(proj_dir + "/" + scene_name, output_scene, std::filesystem::copy_options::update_existing);
		}
	}
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
			if (!std::filesystem::exists(rp::utility::working_path() + "/audio")) {
				std::filesystem::create_directories(rp::utility::working_path() + "/audio");
			}
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
			auto [mono_bytes, mono_files] = DiscoverMonoRuntimeFiles();
			total_bytes += mono_bytes;
			file_ct += mono_files;

			std::uint64_t bytes_copied_mul100{};
			std::uint32_t file_copied{};

			if (context->m_state != BuildState::ABORTED) {
				MakeTemplateExecutable(outputDir, config.output_name, config.fullscreen);
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
				std::filesystem::copy_file(curr + "/managed/GameAssembly.dll", precompiledpath+"/GameAssembly.dll", std::filesystem::copy_options::update_existing);
				std::filesystem::copy_file(std::filesystem::current_path().string() + "/bin/BasilEngine.dll", precompiledpath + "/BasilEngine.dll", std::filesystem::copy_options::update_existing);
				std::filesystem::copy_file(std::filesystem::current_path().string() + "/bin/Engine.Bindings.dll", precompiledpath + "/Engine.Bindings.dll", std::filesystem::copy_options::update_existing);
			}
			for (const auto& cde : std::filesystem::recursive_directory_iterator{ rp::utility::output_path() }) {
				if (context->m_state == BuildState::ABORTED)
					return;
				if (!cde.is_directory()) {
					std::string dest = outputDir + "/assets/bin/" + rp::utility::get_relative_path(cde.path().string(), rp::utility::output_path());
					std::filesystem::path parent = std::filesystem::path(dest).parent_path();
					if (!std::filesystem::exists(parent)) {
						std::filesystem::create_directories(parent);
					}
					std::filesystem::copy_file(cde,dest, std::filesystem::copy_options::update_existing);
					bytes_copied_mul100 += cde.file_size()*100;
					file_copied++;
					int current_progress = bytes_copied_mul100 / total_bytes;
					if (current_progress > local_progress) {
						context->m_progress100 = local_progress = current_progress;
					}
				}
			}
			std::string assetlist_loc = outputDir + "/assets/bin/.assetlist";
			if (std::filesystem::exists(assetlist_loc)) {
				std::filesystem::rename(assetlist_loc, outputDir+"/.assetlist");
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
					std::filesystem::copy_file(cde, dest, std::filesystem::copy_options::update_existing);
					bytes_copied_mul100 += cde.file_size()*100;
					file_copied++; 
					int current_progress = bytes_copied_mul100 / total_bytes;
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
