#ifndef Engine_HPP
#define Engine_HPP

#include <string>
#include <memory>
#include <spdlog/stopwatch.h>
#include "Ecs/ecs.h"

class Window;
struct RenderSystem;
struct SceneRegistry;
namespace Logger {
	class Sink;
}
struct GLFWwindow;

class Engine
{
	using on_world_load_cb = void(*)(ecs::world&);
	using on_world_unload_cb = void(*)(ecs::world&);

	std::unique_ptr<Window> m_Window;
	std::unique_ptr<RenderSystem> m_RenderSystem;
	ecs::world m_World;
	std::unique_ptr<Logger::Sink> m_Sink;
	std::string m_WorkingDirectory;
	std::unique_ptr<SceneRegistry> m_SceneRegistry;

	on_world_load_cb m_OnWorldLoadCallback{};
	on_world_unload_cb m_OnWorldUnloadCallback{};

public:
	struct Info {

		double m_FPS{};
		double m_DeltaTime{};          // Time elapsed since last frame (seconds)
		double m_LastFrameTime{};      // Time of last frame (for delta calculation)
		std::uint64_t m_TotalFrameCt{};
		std::uint64_t m_FrameLogCounter{};	//basically total (mod) rate
		std::uint64_t m_FrameLogRate;
		spdlog::stopwatch m_StartTime;

		enum class State : std::uint32_t {
			Running,
			Error,
			Wait,
			Pause,
			Init,
			Exit
		} m_State{ State::Running };
	};
private:
	Info m_Info;
	static std::unique_ptr<Engine>& InstancePtr();

public:
	static Engine& Instance();

	static void Init(std::string const& cfg = {});
	static void InitInheritWindow(std::string const& cfg, GLFWwindow*);
	static void InitWithoutWindow(std::string const& cfg = {});
	static void Update();
	static void CoreUpdate();
	static void UpdateDebug();
	static void Exit();

	

	static void BeginFrame();
	static void EndFrame();

	static Window& GetWindowInstance();
	static RenderSystem& GetRenderSystem();
	static SceneRegistry& GetSceneRegistry();
	static spdlog::stopwatch GetAppElapsedTime();
	static Logger::Sink* GetSink();
	static bool WindowShouldClose();

	static void GenerateDefaultConfig();
	//static void CreateDefaultResources();

	static void ReportLastError();
	static ecs::world GetWorld();
	static double GetDeltaTime();

	Info const& GetInfo() const { return m_Info; }
	Info& GetInfo() { return m_Info; }
	
	static Info::State GetState() { return Instance().m_Info.m_State; }
	static void SetState(Info::State state) { Instance().m_Info.m_State = state; }

	static bool ShouldClose() { return Instance().m_Info.m_State == Info::State::Error || Instance().m_Info.m_State == Info::State::Exit; }

	static void OnLoad()
	{
		if (Instance().m_OnWorldLoadCallback)
		{
			Instance().m_OnWorldLoadCallback(Instance().m_World);
		}
	}

	static void OnUnload()
	{
		if (Instance().m_OnWorldUnloadCallback)
		{
			Instance().m_OnWorldUnloadCallback(Instance().m_World);
		}
	}

	static void SetOnLoadCallBack(on_world_load_cb cb)
	{
		Instance().m_OnWorldLoadCallback = cb;
	}
	static void SetOnUnloadCallBack(on_world_unload_cb cb)
	{
		Instance().m_OnWorldUnloadCallback = cb;
	}
};


#endif // Engine_HPP