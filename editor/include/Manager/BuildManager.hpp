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

enum class BuildWindowMode : std::uint8_t {
	windowed,
	fullscreen
};

enum class ResourceCleanUpMode : std::uint8_t {
	none,
	minimal //, //removes hard dependencies
	//all_unused //removes everything unused very slow and dangerous
};

struct WindowDims {
	unsigned int width{ 1600 };
	unsigned int height{ 900 };
};

//simple m4 build system. this will be fleshed out in m5
//yaml build
struct BuildConfiguration {
	std::string output_dir;
	std::string output_name;
	std::string icon_relative_path; //relative path //should warn if dne
	ResourceCleanUpMode resource_cleanup; //limited support for soft dependencies
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

//contract between caller and async callee. ensures caller resource lifetime does not cause problems for callee
struct BuildContext {
	std::atomic<BuildState> m_state;
	std::atomic_int m_progress100;
};

class EditorMain;

struct BuildManager {
	std::future<void> BuildAsync(BuildConfiguration, std::shared_ptr<BuildContext>); //remove buildcontext in the future
	//std::unordered_set<rp::BasicIndexedGuid> DiscoverSceneResources(std::uint64_t* total_sz = nullptr, std::uint32_t* file_ct = nullptr, bool discover_soft_dependencies = false);
	static BuildConfiguration LoadBuildConfiguration();
	static void SaveBuildConfiguration(BuildConfiguration const&);

	EditorMain* m_editor; //opaque ptr
};

#endif