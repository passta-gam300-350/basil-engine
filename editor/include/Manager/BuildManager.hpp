#ifndef EDITOR_BUILD_MANAGER_H
#define EDITOR_BUILD_MANAGER_H

#include <atomic>
#include <memory>
#include <string>
#include <future>

//simple m4 build system. this will be fleshed out in m5
//yaml build
struct BuildConfiguration {
	std::string output_dir;
	std::string output_name;
	bool strip_unused_assets; //not supported in m4
	bool fullscreen;
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

//stateless for now (simple impl)
struct BuildManager {
	static std::future<void> BuildAsync(BuildConfiguration, std::shared_ptr<BuildContext>);
	static BuildConfiguration LoadBuildConfiguration();
	static void SaveBuildConfiguration(BuildConfiguration const&);
};

#endif