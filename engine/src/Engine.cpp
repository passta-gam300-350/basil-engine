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
#include "Particles/ParticleSystem.h"
#include "Render/Camera.h"
#include "System/Audio.hpp"
#include "System/HierarchySystem.hpp"

#include "Scene/Scene.hpp"
#include "Resources/Texture.h"
#include <array>
#include <glad/glad.h>
#include "Render/VideoPlayback.hpp"
#include "System/AnimationSystem.hpp"
#include "Manager/ResourceSystem.hpp"

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
	
	static std::unique_ptr<AudioComponent> ambient; // [TEMP]
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
	std::string playerDir = std::filesystem::absolute(std::filesystem::path{"."}).string();
	Engine::setWorkingDir(playerDir.c_str());

	Instance().m_Info.m_FrameLogRate = 165;

	if (!std::filesystem::exists(cfg))
	{
		GenerateDefaultConfig();
	}
	YAML::Node root{ YAML::LoadFile(cfg.empty() ? DEFAULT_CONFIG_NAME.data() : cfg) };
	if (YAML::Node window{ root["window"] }; window) {
		std::uint32_t win_width{ window["width"] ? window["width"].as<std::uint32_t>() : DEFAULT_RESOLUTION_WIDTH };
		std::uint32_t win_height{ window["height"] ? window["height"].as<std::uint32_t>() : DEFAULT_RESOLUTION_HEIGHT };
		bool win_mode{ window["fullscreen"] ? window["fullscreen"].as<bool>() : DEFAULT_WINDOW_MODE };
		std::string win_name{ window["title"] ? window["title"].as<std::string>() : std::string(DEFAULT_NAME.begin(), DEFAULT_NAME.end()) };
		bool win_vsync{ window["vsync"] ? window["vsync"].as<bool>() : DEFAULT_VSYNC_OPTION };
		Instance().m_Window = std::make_unique<Window>(win_name, win_width, win_height);
		Instance().m_Window->SetVSync(win_vsync);

		if (win_mode) {
			auto ptr = Instance().m_Window->GetNativeWindow();
			GLFWmonitor* monitor = glfwGetPrimaryMonitor(); // Get the monitor's video mode (resolution, refresh rate, etc.) const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			// Switch to fullscreen
			glfwSetWindowMonitor(ptr, monitor, 0, 0, win_width, win_height, 0);
		}
	}
	else {
		Instance().m_Window = std::make_unique<Window>(DEFAULT_NAME.data(), DEFAULT_RESOLUTION_WIDTH, DEFAULT_RESOLUTION_HEIGHT);
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
		std::string sink_name{ logger["name"] ? logger["name"].as<std::string>() : DEFAULT_SINK_NAME };
		std::string sink_file{ logger["output file"] ? logger["output file"].as<std::string>() : std::string{} };
		spdlog::level::level_enum sink_severity{ logger["severity"] ? static_cast<spdlog::level::level_enum>(logger["severity"].as<int>()) : spdlog::level::info };
		Instance().m_Sink.reset(new Logger::Sink{ sink_name, sink_file, sink_severity });
	}
	else {
		Instance().m_Sink.reset(new Logger::Sink{ DEFAULT_SINK_NAME.data(), std::string{} });
	}
	ReflectionRegistry::SetupNativeTypes();
	ReflectionRegistry::SetupEngineTypes();

	Instance().m_World = WorldRegistry::NewWorld();

	Instance().m_SceneRegistry = std::make_unique<SceneRegistry>();

	// Create and initialize RenderSystem
	Instance().m_RenderSystem = std::make_unique<RenderSystem>();
	Instance().m_RenderSystem->Init();

	// Set up RenderSystem observers
	Instance().m_RenderSystem->SetupComponentObservers(Instance().m_World);

	// Initialize MaterialOverridesSystem (depends on RenderSystem being fully initialized)
	MaterialOverridesSystem::Instance().Init();

	ParticleSystem::GetInstance().setRenderer(Engine::GetRenderSystem().GetSceneRenderer());

	PhysicsSystem::Instance().Init();
	PhysicsSystem::Instance().SetupObservers();

	// Initialize Jolt debug renderer AFTER PhysicsSystem::Init (Jolt must be initialized first)
	Instance().m_RenderSystem->InitJoltDebugRenderer();

	// [TEMP]
	AudioSystem::GetInstance().Init();
	VideoSystem().Init();

	std::string manifest_path = std::string{ Engine::getWorkingDir() } + "/scene_manifest.order";
	Instance().GetSceneRegistry().ReadManifest(manifest_path);
	Instance().GetSceneRegistry().RequestSceneChange(0);

	//InputManager::Get_Instance()->Setup_Callbacks();
	MonoEntityManager::GetInstance().SetPreCompiled(true);
	MonoEntityManager::GetInstance().initialize();
	MonoEntityManager::GetInstance().StartCompilation();

	BehaviourSystem::Instance().isActive = true;
	BehaviourSystem::Instance().Init();

	BindingSystem::RegisterBindings();
	Scheduler::CompileJobSchedule();

	InputManager::Setup_Callbacks();

	Engine::SetOnLoadCallBack([](ecs::world& w)
		{
			BehaviourSystem::Instance().Reload();

		});

	Engine::SetOnUnloadCallBack([](ecs::world& w)
		{

			BehaviourSystem::Instance().firstRun = true;
			BehaviourSystem::Instance().unloaded = true;
		});

	auto e = Engine::GetWorld().add_entity();
	e.destroy();

	Engine::Instance().m_Info.m_State = Info::State::Running;
}

void Engine::CoreUpdate() {
	Engine::Instance().m_Info.m_StartTime.reset();
	//thread_local auto physic_system{PhysicsSystem()};
	Engine& instance{ Instance() };
	//PF_BEGIN_FRAME(instance.m_Info.m_TotalFrameCt);
	instance.m_World.pre_update();
	//TransformSystem().FixedUpdate(instance.m_World);
	HierarchySystem().FixedUpdate(instance.m_World);
	CameraSystem::Instance().FixedUpdate(instance.m_World);
	MaterialOverridesSystem::Instance().Update(instance.m_World, 0.0f); // Sync MaterialOverridesComponent -> MaterialInstance
	//physic_system.FixedUpdate(instance.m_World);
	//instance.m_World.update();
	//JobID last_job{ instance.m_World.update_async()};
	animationSystem().FixedUpdate(instance.m_World);
	PhysicsSystem::Instance().FixedUpdate(instance.m_World);
	ParticleSystem::GetInstance().Update(instance.m_World, float(instance.GetLastDeltaTime()));

	// Unity-style: Sync active scene's render settings (skybox, etc.) to renderer
	Engine::SyncActiveSceneRenderSettings();
	VideoSystem().Update(instance.m_World);

	Engine::GetRenderSystem().Update(instance.m_World);
	//Scheduler::Instance().m_JobSystem.wait_for(last_job);
	//messagingSystem.Publish(MessageID::ENGINE_CORE_UPDATE_COMPLETE, std::make_unique<NullMessage>());
	//messagingSystem.Update();
	AudioSystem::GetInstance().Update(instance.m_World); // [TEMP]
	//PF_END_FRAME();
	BehaviourSystem::Instance().Update(instance.m_World, float(instance.GetLastDeltaTime()));
	InputManager::Get_Instance()->Update();
	messagingSystem.Update();


	instance.m_SceneRegistry->PollRequestSceneChange();
	instance.m_Info.m_ActualDeltaTime = instance.m_Info.m_StartTime.elapsed().count();
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
				{
					CoreUpdate();
					UpdateDebug();
					Engine::GetWindowInstance().SwapBuffers();
				}
				
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
	PF_SYSTEM("Update Debug");
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
	if (!std::filesystem::exists(cfg))
	{
		GenerateDefaultConfig();
	}
	InitWithoutWindow(cfg);
}

void Engine::InitWithoutWindow(std::string const& cfg, bool is_precompiled) {
	Engine::SetState(Info::State::Init);
	Engine::SetOnLoadCallBack([](ecs::world& world) {
		spdlog::info("World loaded with {} entities", world.get_all_entities().size());
	});
	Engine::SetOnUnloadCallBack([](ecs::world& world)
	{
		spdlog::info("World is unloaded.");

	});

	ReflectionRegistry::SetupNativeTypes();
	ReflectionRegistry::SetupEngineTypes();

	MonoEntityManager::GetInstance().SetPreCompiled(is_precompiled);
	MonoEntityManager::GetInstance().StartCompilation();

	Instance().m_Info.m_FrameLogRate = 165;
	
	Instance().m_World = WorldRegistry::NewWorld();

	if (!cfg.empty()) {
		Engine::GenerateDefaultConfig();
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

	ParticleSystem::GetInstance().setRenderer(Engine::GetRenderSystem().GetSceneRenderer());

	Instance().m_SceneRegistry = std::make_unique<SceneRegistry>();

	BindingSystem::RegisterBindings();

	PhysicsSystem::Instance().Init();
	PhysicsSystem::Instance().SetupObservers();

	// Initialize Jolt debug renderer AFTER PhysicsSystem::Init (Jolt must be initialized first)
	Instance().m_RenderSystem->InitJoltDebugRenderer();

	Scheduler::CompileJobSchedule();

	// [TEMP]
	AudioSystem::GetInstance().Init();
	VideoSystem().Init();
	///*for (const auto& entry : std::filesystem::directory_iterator(AUDIO_PATH)) {
	//	if (entry.is_regular_file() && entry.path().extension() == AUDIO_EXTENSION) {
	//		std::string filename = entry.path().stem().string();
	//		AudioSystem::GetInstance().LoadSound((AUDIO_PATH + filename + AUDIO_EXTENSION), true, false, true);
	//	}
	//	else
	//		spdlog::warn("Audio: File extension for {} is not {}!", entry.path().filename().string(), AUDIO_EXTENSION);
	//}*/
	// Persist the ambient audio component so it doesn't get destroyed after init
	/*ambient = std::make_unique<AudioComponent>();
	if (ambient->Init("Singapore Koi Ambience_Loop.ogg", true, true, true)) {
		ambient->Play();
		ambient->SetVolume(6.0f);
	}*/
	// [ENDTEMP]


	auto e = Engine::GetWorld().add_entity();
	e.destroy();

	Engine::SetState(Info::State::Running);
}

void Engine::Exit() {
	SystemRegistry::Exit();
	InputManager::Get_Instance()->Destroy_Instance();
	if (Instance().m_RenderSystem) {
		Instance().m_RenderSystem->Exit();
		Instance().m_RenderSystem.reset();
	}
	ResourceSystem::Release();
	Scheduler::Release();
	AudioSystem::GetInstance().Exit(); // [TEMP]
	InstancePtr().reset();
	MonoEntityManager::GetInstance().ClearAll();
	WorldRegistry::Clear();
}

world Engine::GetWorld() {
	return Instance().m_World;
}

double Engine::GetDeltaTime() {
	return Instance().m_Info.m_DeltaTime;
}

double Engine::GetLastDeltaTime() {
	return Instance().m_Info.m_ActualDeltaTime;
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

SceneRegistry& Engine::GetSceneRegistry()
{
	if (!Instance().m_SceneRegistry) {
		throw std::runtime_error("SceneRegistry not created - call Engine::Init() first");
	}
	return *Instance().m_SceneRegistry;
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

void Engine::SyncActiveSceneRenderSettings()
{
	PF_SYSTEM("SyncActiveSceneRenderSettings");
	// Unity-style: Apply active scene's render settings to the renderer
	if (!Instance().m_SceneRegistry || !Instance().m_RenderSystem) {
		return; // Not yet initialized
	}

	auto activeSceneOpt = Instance().m_SceneRegistry->GetActiveScene();
	if (!activeSceneOpt.has_value()) {
		return; // No active scene
	}

	Scene& activeScene = activeSceneOpt.value().get();
	auto& settings = activeScene.GetRenderSettings();
	auto& renderSystem = *Instance().m_RenderSystem;

	// Load/reload skybox cubemap if needed (resource pipeline integration)
	if (settings.skybox.needsReload && settings.skybox.enabled) {
		// Check if all face texture GUIDs are valid
		bool allGuidsValid = true;
		for (const auto& guid : settings.skybox.faceTextures) {
			if (guid == rp::null_guid) {
				allGuidsValid = false;
				break;
			}
		}

		if (allGuidsValid) {
			try {
				auto& registry = ResourceRegistry::Instance();
				std::array<unsigned int, 6> textureIDs;
				bool loadSuccess = true;

				// Load all 6 texture resources by GUID
				for (int i = 0; i < 6; ++i) {
					auto* texturePtr = registry.Get<std::shared_ptr<Texture>>(settings.skybox.faceTextures[i]);

					if (texturePtr && *texturePtr) {
						textureIDs[i] = (*texturePtr)->id;  // Get OpenGL texture ID
						GetSink()->logger()->debug("Skybox face {} loaded (GUID: {}, GPU ID: {})",
							i, settings.skybox.faceTextures[i].to_hex().substr(0, 8), textureIDs[i]);
					} else {
						GetSink()->logger()->error("Failed to load skybox face {} (GUID: {})",
							i, settings.skybox.faceTextures[i].to_hex().substr(0, 8));
						loadSuccess = false;
						break;
					}
				}

				if (loadSuccess) {
					// Create cubemap from 6 loaded texture IDs
					unsigned int cubemapID = TextureLoader::CubemapFromTextureIDs(textureIDs, true);

					if (cubemapID != 0) {
						// Delete old cubemap if it exists
						if (settings.skybox.cachedCubemapID != 0) {
							glDeleteTextures(1, &settings.skybox.cachedCubemapID);
						}

						settings.skybox.cachedCubemapID = cubemapID;
						settings.skybox.needsReload = false;
						GetSink()->logger()->info("Skybox cubemap created successfully (ID: {})", cubemapID);
					} else {
						GetSink()->logger()->error("Failed to create skybox cubemap from texture IDs");
					}
				}
			}
			catch (const std::exception& e) {
				GetSink()->logger()->error("Exception loading skybox cubemap: {}", e.what());
			}
		} else {
			GetSink()->logger()->debug("Skybox: Not all face textures assigned (skipping load)");
		}
	}

	// Sync skybox settings to renderer
	renderSystem.SetSkyboxCubemap(settings.skybox.cachedCubemapID);
	renderSystem.EnableSkybox(settings.skybox.enabled);
	renderSystem.SetSkyboxExposure(settings.skybox.exposure);
	renderSystem.SetSkyboxRotation(settings.skybox.rotation);
	renderSystem.SetSkyboxTint(settings.skybox.tint);

	// Sync lighting settings to renderer
	renderSystem.m_SceneRenderer->SetAmbientLight(settings.ambientLight);
	renderSystem.m_SceneRenderer->SetBackgroundColor(settings.backgroundColor);

	// Sync post-processing settings to renderer
	renderSystem.m_SceneRenderer->SetBloomStrength(settings.bloomStrength);
	renderSystem.m_SceneRenderer->SetToneMappingMethod(settings.toneMapMethod);
	renderSystem.m_SceneRenderer->SetExposureClampRange(settings.exposureMin, settings.exposureMax);
}
