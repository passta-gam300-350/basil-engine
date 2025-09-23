#ifndef Engine_HPP
#define Engine_HPP

#include <string>
#include <memory>
#include <spdlog/stopwatch.h>
#include "Ecs/ecs.h"

class Window;
namespace Logger {
	class Sink;
}

class Engine
{
	std::unique_ptr<Window> m_Window;
	ecs::world m_World;
	std::unique_ptr<Logger::Sink> m_Sink;
	std::string m_WorkingDirectory;

	struct Info {
		std::uint64_t m_TotalFrameCt;
		std::uint64_t m_FrameLogCounter;	//basically total (mod) rate
		std::uint64_t m_FrameLogRate;
		spdlog::stopwatch m_StartTime;

		enum class State : std::uint32_t {
			Running,
			Error,
			Wait,
			Pause,
			Exit
		} m_State;
	} m_Info;

public:
	static Engine& Instance();

	static void Init(std::string const& cfg = {});
	static void Update();
	static void Exit();

	static Window& GetWindowInstance();
	static spdlog::stopwatch GetAppElapsedTime();
	static Logger::Sink* GetSink();
	static bool WindowShouldClose();

	static void GenerateDefaultConfig();
	static void ReportLastError();

	//engine state management
	void Coma();	//do not use recklessly, this will put the program into deadlock 
	void Terminate();


};


#endif // Engine_HPP