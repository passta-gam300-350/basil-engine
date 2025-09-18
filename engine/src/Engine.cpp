#include "Engine.hpp"
#include "Core/Window.h"
#include "Profiler/profiler.hpp"
#include "Manager/ResourceSystem.hpp"
#include "Ecs/ecs.h"

namespace {
	constexpr std::uint64_t DEFAULT_RESOLUTION_WIDTH{ 1600ul };
	constexpr std::uint64_t DEFAULT_RESOLUTION_HEIGHT{ 900ul };
	constexpr bool DEFAULT_WINDOW_MODE{ false }; // false for windowed, true for fullscreen (borderless window is not support) //nvm both not supported// TODO: expand Core/Window.h Window interface
	constexpr std::string_view DEFAULT_NAME{"Engine"};
	constexpr std::string_view DEFAULT_SINK_NAME{ "Engine" };
}

Engine& Engine::Instance() {
	static Engine instance;
	return instance;
}

using namespace ecs; //lazy

void Engine::Init(std::string const& cfg ) {
	Instance().m_World = WorldRegistry::NewWorld();
	if (cfg.empty()) {
		Instance().m_Window.reset(new Window(DEFAULT_NAME.data(), DEFAULT_RESOLUTION_WIDTH, DEFAULT_RESOLUTION_HEIGHT));
		return;
	}
	YAML::Node root{YAML::LoadFile(cfg)};
	std::uint64_t win_width{ DEFAULT_RESOLUTION_WIDTH };
	std::uint64_t win_height;
	if (YAML::Node window{ root["window"] }; window) {
		std::uint64_t win_width{ window["width"] ? window["width"].as<std::uint64_t>() : DEFAULT_RESOLUTION_WIDTH };
		std::uint64_t win_height{ window["height"] ? window["height"].as<std::uint64_t>() : DEFAULT_RESOLUTION_HEIGHT };
		bool win_mode{ window["fullscreen"] ? window["fullscreen"].as<bool>() : DEFAULT_WINDOW_MODE };
		std::string win_name{ window["title"] ? window["title"].as<std::string>() : DEFAULT_NAME };
		Instance().m_Window.reset(new Window(win_name, win_width, win_height));
	}
	else {
		Instance().m_Window.reset(new Window(DEFAULT_NAME.data(), DEFAULT_RESOLUTION_WIDTH, DEFAULT_RESOLUTION_HEIGHT));
	}
	if (YAML::Node system{ root["system"] }; system) {
		SystemRegistry::LoadConfig(system);
	}
	if (YAML::Node resource{ root["resource"] }; resource) {
		ResourceSystem::LoadConfig(resource);
	}
	if (YAML::Node world_file{ root["world"] && root["world"]["file"]}; world_file) {
		Instance().m_World.LoadYAML(world_file.as<std::string>());
	}
	if (YAML::Node logger{ root["logger"] }; logger) {
		std::string sink_name{ logger["name"] ? logger["name"].as<std::string>() : DEFAULT_SINK_NAME};
		std::string sink_file{ logger["output file"] ? logger["output file"].as<std::string>() : std::string{} };
		spdlog::level::level_enum sink_severity{ logger["severity"] ? static_cast<spdlog::level::level_enum>(logger["severity"].as<int>()) : spdlog::level::info };
		Instance().m_Sink.reset(new Logger::Sink{sink_name, sink_file, sink_severity});
	}
	else {
		Instance().m_Sink.reset(new Logger::Sink{ DEFAULT_SINK_NAME.data(), std::string{}});
	}
}

void Engine::Update() {
	std::uint64_t& frame_number{ Instance().m_Info.m_TotalFrameCt };
	std::uint64_t& frame_counter{ Instance().m_Info.m_FrameLogCounter };
	std::uint64_t& frame_log_rate{ Instance().m_Info.m_FrameLogRate };
	PF_BEGIN_FRAME(frame_number);
	Instance().m_World.update();
	PF_END_FRAME();

	if (frame_log_rate && frame_counter >= frame_log_rate) {
		Profiler::instance().printLastFrameSummary();
	}

	frame_number++;
	frame_counter--;
}

void Engine::Exit() {
	SystemRegistry::Exit();
	WorldRegistry::Clear();
}

Window& Engine::GetWindowInstance() {
	return *Instance().m_Window;
}

bool Engine::WindowShouldClose() {
	return GetWindowInstance().ShouldClose();
}

Logger::Sink* Engine::GetSink() {
	return Instance().m_Sink.get();
}