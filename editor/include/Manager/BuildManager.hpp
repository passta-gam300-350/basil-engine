/******************************************************************************/
/*!
\file   BuildManager.hpp
\author Team PASSTA
		Chew Bangxin Steven (bangxinsteven.chew\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the declaration of the BuildManager class, which
manages the game build.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef EDITOR_BUILD_MANAGER_H
#define EDITOR_BUILD_MANAGER_H

#include <atomic>
#include <memory>
#include <string>
#include <future>
#include <unordered_map>
#include <rsc-core/rp.hpp>

enum class BuildWindowMode : std::uint8_t {
	windowed,
	fullscreen
};

enum class ResourceCleanUpMode : std::uint8_t {
	none,
	minimal
};

struct WindowDims {
	unsigned int width{ 1600 };
	unsigned int height{ 900 };
};

struct BuildConfiguration {
	std::string output_dir;
	std::string output_name;
	std::string icon_relative_path;
	ResourceCleanUpMode resource_cleanup;
	BuildWindowMode windowing_mode;
	WindowDims window_size;
};

enum class BuildState : std::uint8_t {
	IDLE,
	QUEUED,
	IN_PROGRESS,
	SUCCESS,
	FAILED,
	ABORTED
};

enum class BuildPhase : std::uint8_t {
	Idle,
	DiscoveringResources,
	CompilingScripts,
	CreatingExecutable,
	CopyingMonoRuntime,
	CopyingManagedDLLs,
	PackagingResources,
	CopyingAudio,
	Done
};

inline const char* BuildPhaseLabel(BuildPhase phase) {
	switch (phase) {
	case BuildPhase::DiscoveringResources:  return "Discovering resources...";
	case BuildPhase::CompilingScripts:       return "Compiling scripts...";
	case BuildPhase::CreatingExecutable:     return "Creating executable...";
	case BuildPhase::CopyingMonoRuntime:     return "Copying Mono runtime...";
	case BuildPhase::CopyingManagedDLLs:     return "Copying managed DLLs...";
	case BuildPhase::PackagingResources:     return "Packaging resources...";
	case BuildPhase::CopyingAudio:           return "Copying audio...";
	case BuildPhase::Done:                   return "Done!";
	default:                                 return "Preparing...";
	}
}

struct BuildContext {
	std::atomic<BuildState> m_state{BuildState::IDLE};
	std::atomic_int m_progress100{0};
	std::atomic<BuildPhase> m_phase{BuildPhase::Idle};
	std::atomic_uint32_t m_scenes_discovered{0};
	std::atomic_uint32_t m_scenes_total{0};
	std::atomic_uint32_t m_resources_found{0};
	std::atomic_uint32_t m_files_copied{0};
	std::atomic_uint32_t m_files_total{0};
	std::string m_output_path;
};

struct DescriptorInfo {
	std::string desc_path;
	std::uint64_t importer_type;
};

using DescriptorIndex = std::unordered_map<rp::Guid, DescriptorInfo>;

struct BuildManager {
	std::future<void> BuildAsync(BuildConfiguration, std::shared_ptr<BuildContext>);
	static int BuildSync(BuildConfiguration, std::string projectDir, std::string outputDir);

	static DescriptorIndex BuildDescriptorIndex(std::string const& assetsDir);
	static BuildConfiguration LoadBuildConfiguration();
	static BuildConfiguration LoadBuildConfigurationFrom(std::string const& projectDir);
	static void SaveBuildConfiguration(BuildConfiguration const&);
};

#endif
