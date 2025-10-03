#include "Engine.hpp"
#include "Core/Window.h"
#include "Profiler/profiler.hpp"
#include "Manager/ResourceSystem.hpp"
#include "Input/InputManager.h"
#include "Ecs/ecs.h"
#include "Render/render.h"
#include <stdexcept>

#ifdef _WIN32
// NVIDIA Optimus - force discrete GPU
extern "C" {
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
}

// AMD PowerXpress - force discrete GPU  
extern "C" {
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif
#include "Manager/ObjectManager.hpp"

namespace {
	constexpr std::uint32_t DEFAULT_RESOLUTION_WIDTH{ 1600ul };
	constexpr std::uint32_t DEFAULT_RESOLUTION_HEIGHT{ 900ul };
	constexpr bool DEFAULT_WINDOW_MODE{ false }; // false for windowed, true for fullscreen (borderless window is not support) //nvm both not supported// TODO: expand Core/Window.h Window interface
	constexpr bool DEFAULT_VSYNC_OPTION{ false }; //true is toggle
	constexpr std::uint32_t DEFAULT_LOG_SEVERITY{spdlog::level::info};
	constexpr std::string_view DEFAULT_NAME{"Engine"};
	constexpr std::string_view DEFAULT_SINK_NAME{ "Engine" };
	constexpr std::string_view DEFAULT_OUTPUT_FILE{ "" };
	constexpr std::string_view DEFAULT_CONFIG_NAME{ "Default.yaml"};
}

Engine& Engine::Instance() {
	static Engine instance;
	return instance;
}

using namespace ecs; //lazy

void Engine::Init(std::string const& cfg ) {

	Instance().m_Info.m_FrameLogRate = 165;

	ReflectionRegistry::SetupNativeTypes();
	Instance().m_World = WorldRegistry::NewWorld();
	if (cfg.empty()) {
		Instance().m_Window = std::make_unique<Window>(DEFAULT_NAME.data(), DEFAULT_RESOLUTION_WIDTH, DEFAULT_RESOLUTION_HEIGHT);
		//InputManager::Get_Instance()->Setup_Callbacks();
		//ObjectManager::GetInstance().CreateGameObject();
		return;
	}
	YAML::Node root{YAML::LoadFile(cfg)};
	if (YAML::Node window{ root["window"] }; window) {
		std::uint32_t win_width{ window["width"] ? window["width"].as<std::uint32_t>() : DEFAULT_RESOLUTION_WIDTH };
		std::uint32_t win_height{ window["height"] ? window["height"].as<std::uint32_t>() : DEFAULT_RESOLUTION_HEIGHT };
		//bool win_mode{ window["fullscreen"] ? window["fullscreen"].as<bool>() : DEFAULT_WINDOW_MODE };
		std::string win_name{ window["title"] ? window["title"].as<std::string>() : std::string(DEFAULT_NAME.begin(), DEFAULT_NAME.end()) };
		bool win_vsync{ window["vsync"] ? window["vsync"].as<bool>() : DEFAULT_VSYNC_OPTION };
		Instance().m_Window = std::make_unique<Window>(win_name, win_width, win_height);
		Instance().m_Window->SetVSync(win_vsync);
	}
	else {
		Instance().m_Window = std::make_unique<Window>(DEFAULT_NAME.data(), DEFAULT_RESOLUTION_WIDTH, DEFAULT_RESOLUTION_HEIGHT);
	}
	if (YAML::Node system{ root["system"] }; system) {
		SystemRegistry::LoadConfig(system);
	}
	if (YAML::Node resource{ root["resource"] }; resource) {
		ResourceSystem::LoadConfig(resource);
	}
	if (root["world"]) {
		if (YAML::Node world_file{ root["world"]["file"] }; world_file) {
			Instance().m_World.LoadYAML(world_file.as<std::string>());
		}
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

	RenderSystem::System().Init();
	//InputManager::Get_Instance()->Setup_Callbacks();
	Scheduler::CompileJobSchedule();

	
}

void Engine::Update() {
	try {
		Engine& instance{ Instance() };
		std::uint64_t& frame_number{ instance.m_Info.m_TotalFrameCt }; 
		while (instance.m_Info.m_State != Info::State::Error && instance.m_Info.m_State != Info::State::Exit) {
			while (instance.m_Info.m_State == Info::State::Running) {
				if (Engine::GetWindowInstance().PollEvents(); Engine::GetWindowInstance().ShouldClose()) {
					instance.m_Info.m_State = Info::State::Exit;
					break;
				}
				
				InputManager::Get_Instance()->Update();

				PF_BEGIN_FRAME(frame_number);
				//instance.m_World.update();
				RenderSystem::System().Update(instance.m_World);
				PF_END_FRAME();
				Engine::GetWindowInstance().SwapBuffers();




				UpdateDebug();
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

void Engine::UpdateDebug() {

	Engine& instance{ Instance() };
	std::uint64_t& frame_number{ instance.m_Info.m_TotalFrameCt };
	std::uint64_t& frame_counter{ instance.m_Info.m_FrameLogCounter };
	std::uint64_t& frame_log_rate{ instance.m_Info.m_FrameLogRate };



	if (frame_log_rate && frame_counter >= frame_log_rate) {
		Profiler::instance().printLastFrameSummary();
		Engine::Instance().GetInfo().m_FPS = Profiler::instance().getLastFps();

	}

	frame_number++;
	frame_counter--;
}

void Engine::ReportLastError() {
	
}

void Engine::InitWithoutWindow(std::string const& cfg) {
	ReflectionRegistry::SetupNativeTypes();
	Instance().m_Info.m_FrameLogRate = 165;
	
	Instance().m_World = WorldRegistry::NewWorld();

	if (!cfg.empty()) {
		YAML::Node root{YAML::LoadFile(cfg)};

		if (YAML::Node system{ root["system"] }; system) {
			SystemRegistry::LoadConfig(system);
		}
		if (YAML::Node resource{ root["resource"] }; resource) {
			ResourceSystem::LoadConfig(resource);
		}
		if (root["world"]) {
			if (YAML::Node world_file{ root["world"]["file"] }; world_file) {
				Instance().m_World.LoadYAML(world_file.as<std::string>());
			}
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
	} else {
		Instance().m_Sink.reset(new Logger::Sink{ DEFAULT_SINK_NAME.data(), std::string{}});
	}

	RenderSystem::System().Init();
	Scheduler::CompileJobSchedule();
}

void Engine::Exit() {
	SystemRegistry::Exit();
	WorldRegistry::Clear();
	InputManager::Get_Instance()->Destroy_Instance();
	RenderSystem::System().Exit();
	ResourceSystem::Release();
	Scheduler::Release();
}

world Engine::GetWorld() {
	return Instance().m_World;
}

void Engine::GenerateDefaultConfig() {
	YAML::Node root{  };
	YAML::Node window{  };
	YAML::Node logger{  };
	window["width"] = DEFAULT_RESOLUTION_WIDTH;
	window["height"] = DEFAULT_RESOLUTION_HEIGHT;
	window["fullscreen"] = DEFAULT_WINDOW_MODE;
	window["title"] = std::string(DEFAULT_NAME);
	window["vsync"] = DEFAULT_VSYNC_OPTION;
	logger["name"] = std::string(DEFAULT_SINK_NAME);
	logger["output file"] = std::string(DEFAULT_OUTPUT_FILE);
	logger["severity"] = DEFAULT_LOG_SEVERITY;
	root["window"] = window;
	root["system"] = SystemRegistry::GetDefaultConfig();
	root["resource"] = ResourceSystem::GetDefaultConfig();
	root["logger"] = logger;
	std::ofstream ofs{ DEFAULT_CONFIG_NAME.data() };
	ofs << root;
}

Window& Engine::GetWindowInstance() {
	if (!Instance().m_Window) {
		throw std::runtime_error("Engine window not created - use Engine::Init() instead of InitWithoutWindow() for standalone applications");
	}
	return *Instance().m_Window;
}

bool Engine::WindowShouldClose() {
	return GetWindowInstance().ShouldClose();
}

Logger::Sink* Engine::GetSink() {
	return Instance().m_Sink.get();
}


void Engine::BeginFrame()
{
	PF_BEGIN_FRAME(Instance().m_Info.m_TotalFrameCt);
}

void Engine::EndFrame()
{
	PF_END_FRAME();
}
