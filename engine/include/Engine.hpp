#ifndef Engine_HPP
#define Engine_HPP

#include <string>
#include <memory>
#include <spdlog/stopwatch.h>
#include "Ecs/ecs.h"

class Window;
struct RenderSystem;
namespace Logger {
	class Sink;
}

class Engine
{
	std::unique_ptr<Window> m_Window;
	std::unique_ptr<RenderSystem> m_RenderSystem;
	ecs::world m_World;
	std::unique_ptr<Logger::Sink> m_Sink;
	std::string m_WorkingDirectory;

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
			Exit
		} m_State{ State::Running };
	} m_Info;

public:
	static Engine& Instance();

	static void Init(std::string const& cfg = {});
	static void InitWithoutWindow(std::string const& cfg = {});
	static void Update();
	static void UpdateDebug();
	static void Exit();



	static void BeginFrame();
	static void EndFrame();

	static Window& GetWindowInstance();
	static RenderSystem& GetRenderSystem();
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

	//engine state management
	void Coma();	//do not use recklessly, this will put the program into deadlock 
	void Terminate();
};


#endif // Engine_HPP