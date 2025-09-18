#include "Engine.hpp"
#include "Core/Window.h"
#include "Profiler/profiler.hpp"
#include "Manager/ResourceSystem.hpp"
#include "Input/InputManager.h"
#include "Ecs/ecs.h"

namespace {
	constexpr std::uint64_t DEFAULT_RESOLUTION_WIDTH{ 1600ul };
	constexpr std::uint64_t DEFAULT_RESOLUTION_HEIGHT{ 900ul };
	constexpr bool DEFAULT_WINDOW_MODE{ false }; // false for windowed, true for fullscreen (borderless window is not support) //nvm both not supported// TODO: expand Core/Window.h Window interface
	constexpr bool DEFAULT_VSYNC_OPTION{ false }; //true is toggle
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
		InputManager::Get_Instance()->Setup_Callbacks();
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
		bool win_vsync{ window["vsync"] ? window["vsync"].as<bool>() : DEFAULT_VSYNC_OPTION };
		Instance().m_Window.reset(new Window(win_name, win_width, win_height));
		Instance().m_Window->SetVSync(win_vsync);
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
	InputManager::Get_Instance()->Setup_Callbacks();
}

void Engine::Update() {
	try {
		Engine& instance{ Instance() };
		std::uint64_t& frame_number{ instance.m_Info.m_TotalFrameCt };
		std::uint64_t& frame_counter{ instance.m_Info.m_FrameLogCounter };
		std::uint64_t& frame_log_rate{ instance.m_Info.m_FrameLogRate };
		while (instance.m_Info.m_State != Info::State::Error && instance.m_Info.m_State != Info::State::Exit) {
			while (instance.m_Info.m_State == Info::State::Running) {
				if (Engine::GetWindowInstance().PollEvents(); Engine::GetWindowInstance().ShouldClose()) {
					instance.m_Info.m_State = Info::State::Exit;
					break;
				}
				
				InputManager::Get_Instance()->Update();

				PF_BEGIN_FRAME(frame_number);
				instance.m_World.update();
				PF_END_FRAME();

				if (frame_log_rate && frame_counter >= frame_log_rate) {
					Profiler::instance().printLastFrameSummary();
				}

				frame_number++;
				frame_counter--;
			}
		}
		ReportLastError();					//this is the intended error handler
	}
	catch (std::exception const& e) {		//this is the unintended exception handling, from stl and 3rd party that is unhandled by the engine
		GetSink()->logger()->critical("An error occured! Exception thrown {}", e.what());
	}
	catch (...) {							//this is for whatever alien
		GetSink()->logger()->critical("Fatal error! Caught unknown exception");
	}
}

void Engine::Exit() {
	SystemRegistry::Exit();
	WorldRegistry::Clear();
	InputManager::Get_Instance()->Destroy_Instance();
}


void Engine::GenerateDefaultConfig() {

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