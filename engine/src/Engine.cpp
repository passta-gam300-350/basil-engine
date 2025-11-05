#include "Engine.hpp"
#include "Core/Window.h"
#include "Render/Render.h"
#include "Profiler/profiler.hpp"
#include "Manager/ResourceSystem.hpp"
#include "Input/InputManager.h"
#include "Messaging/Messaging_System.h"
#include "Ecs/ecs.h"
#include "System/BehaviourSystem.hpp"
#include <stdexcept>
#include "System/TransformSystem.hpp"
#include "System/MaterialOverridesSystem.hpp"
#include "Render/Camera.h"

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
#include "Manager/MonoEntityManager.hpp"
#include <System/BindingSystem.hpp>

#include <Physics/Physics_System.h>

namespace {
	constexpr std::uint32_t DEFAULT_RESOLUTION_WIDTH{ 1600ul };
	constexpr std::uint32_t DEFAULT_RESOLUTION_HEIGHT{ 900ul };
	constexpr bool DEFAULT_WINDOW_MODE{ false }; 
	constexpr bool DEFAULT_VSYNC_OPTION{ false }; //true is toggle
	constexpr std::uint32_t DEFAULT_LOG_SEVERITY{spdlog::level::info};
	constexpr std::string_view DEFAULT_NAME{"Engine"};
	constexpr std::string_view DEFAULT_SINK_NAME{ "Engine" };
	constexpr std::string_view DEFAULT_OUTPUT_FILE{ "" };
	constexpr std::string_view DEFAULT_CONFIG_NAME{ "Default.yaml"};
}

Engine& Engine::Instance() {
	return *InstancePtr();
}

std::unique_ptr<Engine>& Engine::InstancePtr() {
	static std::unique_ptr<Engine> s_instance{std::make_unique<Engine>()};
	return s_instance;
}

using namespace ecs; //lazy

void Engine::Init(std::string const& cfg ) {

	Engine::SetState(Info::State::Init);

	Instance().m_Info.m_FrameLogRate = 165;

	ReflectionRegistry::SetupNativeTypes();
	ReflectionRegistry::SetupEngineTypes();
	Instance().m_World = WorldRegistry::NewWorld();
	if (cfg.empty()) {
		Instance().m_Window = std::make_unique<Window>(DEFAULT_NAME.data(), DEFAULT_RESOLUTION_WIDTH, DEFAULT_RESOLUTION_HEIGHT);

		// Create and initialize RenderSystem
		Instance().m_RenderSystem = std::make_unique<RenderSystem>();
		Instance().m_RenderSystem->Init();
		Instance().m_RenderSystem->SetupComponentObservers(Instance().m_World);

		// Initialize MaterialOverridesSystem (depends on RenderSystem being fully initialized)
		MaterialOverridesSystem::Instance().Init();

		Scheduler::CompileJobSchedule();
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

	// Create and initialize RenderSystem
	Instance().m_RenderSystem = std::make_unique<RenderSystem>();
	Instance().m_RenderSystem->Init();

	// Set up RenderSystem observers
	Instance().m_RenderSystem->SetupComponentObservers(Instance().m_World);

	// Initialize MaterialOverridesSystem (depends on RenderSystem being fully initialized)
	MaterialOverridesSystem::Instance().Init();

	//InputManager::Get_Instance()->Setup_Callbacks();
	MonoEntityManager::GetInstance().SetPreCompiled(true);
	MonoEntityManager::GetInstance().initialize();
	MonoEntityManager::GetInstance().StartCompilation();
	BindingSystem::RegisterBindings();
	Scheduler::CompileJobSchedule();
	Engine::Instance().m_Info.m_State = Info::State::Running;
}

void Engine::CoreUpdate() {
	//thread_local auto physic_system{PhysicsSystem()};
	Engine& instance{ Instance() };
	//PF_BEGIN_FRAME(instance.m_Info.m_TotalFrameCt);
	InputManager::Get_Instance()->Update();
	instance.m_World.pre_update();
	TransformSystem().FixedUpdate(instance.m_World);
	CameraSystem::Instance().FixedUpdate(instance.m_World);
	MaterialOverridesSystem::Instance().Update(instance.m_World, 0.0f); // Sync MaterialOverridesComponent -> MaterialInstance
	//physic_system.FixedUpdate(instance.m_World);
	//instance.m_World.update();
	//JobID last_job{ instance.m_World.update_async()};
	PhysicsSystem::Instance().FixedUpdate(instance.m_World);
	Engine::GetRenderSystem().Update(instance.m_World);
	//Scheduler::Instance().m_JobSystem.wait_for(last_job);
	//messagingSystem.Publish(MessageID::ENGINE_CORE_UPDATE_COMPLETE, std::make_unique<NullMessage>());
	//messagingSystem.Update();
	//PF_END_FRAME();
	BehaviourSystem::Instance().Update(instance.m_World, instance.GetDeltaTime());
}

void Engine::Update() {
	try {
		Engine& instance{ Instance() };
		//std::uint64_t& frame_number{ instance.m_Info.m_TotalFrameCt }; 
		while (instance.m_Info.m_State != Info::State::Error && instance.m_Info.m_State != Info::State::Exit) {
			while (instance.m_Info.m_State == Info::State::Running) {
				if (Engine::GetWindowInstance().PollEvents(); Engine::GetWindowInstance().ShouldClose()) {
					instance.m_Info.m_State = Info::State::Exit;
					break;
				}
				
				CoreUpdate();

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
		Engine::Instance().GetInfo().m_DeltaTime = 1 / Engine::Instance().GetInfo().m_FPS;
	}

	frame_number++;
	frame_counter--;

	//TODO: DEBUG REMOVE LATER
	//BehaviourSystem::Instance().Update(instance.m_World, 0.f);
}

void Engine::ReportLastError() {
	
}


void Engine::InitInheritWindow(std::string const& cfg, GLFWwindow* wptr) {
	Engine::SetState(Info::State::Init);
	Instance().m_Window = std::make_unique<Window>(wptr);

	InitWithoutWindow(cfg);
}

void Engine::InitWithoutWindow(std::string const& cfg) {
	Engine::SetState(Info::State::Init);

	ReflectionRegistry::SetupNativeTypes();
	ReflectionRegistry::SetupEngineTypes();

	MonoEntityManager::GetInstance().SetPreCompiled(false);
	MonoEntityManager::GetInstance().StartCompilation();

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

	// Create and initialize RenderSystem
	Instance().m_RenderSystem = std::make_unique<RenderSystem>();
	Instance().m_RenderSystem->Init();

	// Set up RenderSystem observers
	Instance().m_RenderSystem->SetupComponentObservers(Instance().m_World);

	

	
	BindingSystem::RegisterBindings();

	PhysicsSystem::Instance().Init();

	Scheduler::CompileJobSchedule();

	auto e = Engine::GetWorld().add_entity();
	e.destroy();

	Engine::SetState(Info::State::Running);
}

void Engine::Exit() {
	SystemRegistry::Exit();
	WorldRegistry::Clear();
	InputManager::Get_Instance()->Destroy_Instance();
	if (Instance().m_RenderSystem) {
		Instance().m_RenderSystem->Exit();
		Instance().m_RenderSystem.reset();
	}
	ResourceSystem::Release();
	Scheduler::Release();
	InstancePtr().reset();
}

world Engine::GetWorld() {
	return Instance().m_World;
}

double Engine::GetDeltaTime() {
	return Instance().m_Info.m_DeltaTime;
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

RenderSystem& Engine::GetRenderSystem() {
	if (!Instance().m_RenderSystem) {
		throw std::runtime_error("RenderSystem not created - call Engine::Init() first");
	}
	return *Instance().m_RenderSystem;
}

bool Engine::WindowShouldClose() {
	return GetWindowInstance().ShouldClose();
}

Logger::Sink* Engine::GetSink() {
	return Instance().m_Sink.get();
}


void Engine::BeginFrame()
{
	Engine& instance = Instance();

	PF_BEGIN_FRAME(instance.m_Info.m_TotalFrameCt);
}

void Engine::EndFrame()
{
	PF_END_FRAME();
}
