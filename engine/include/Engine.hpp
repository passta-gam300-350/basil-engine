#ifndef Engine_HPP
#define Engine_HPP

#include <string>
#include <memory>
#include <spdlog/stopwatch.h>
#include <glm/glm.hpp>
#include "Ecs/ecs.h"

class Window;
struct RenderSystem;
namespace Logger {
	class Sink;
}
struct GLFWwindow;

// Simple data structures for editor-engine communication (no Graphics library dependencies)
struct PickingResultData {
	bool hasHit{false};
	uint32_t objectID{0};
	glm::vec3 worldPosition{0.0f};
	float depth{0.0f};
};

struct FrameTextureData {
	uint32_t textureID{0};
	bool isValid{false};
	float deltaTime{0.0f};
};

class Engine
{
	std::unique_ptr<Window> m_Window;
	std::unique_ptr<RenderSystem> m_RenderSystem;
	ecs::world m_World;
	std::unique_ptr<Logger::Sink> m_Sink;
	std::string m_WorkingDirectory;

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

	// Editor facade methods - hide Graphics library details from editor
	static void ProcessObjectPicking(int screenX, int screenY, int viewportWidth, int viewportHeight, PickingResultData& outResult);
	static void SetAmbientLightColor(float r, float g, float b);
	static void SetAABBVisualization(bool show);
	static void GetEditorFrameTexture(FrameTextureData& outData);
	static void SetSelectedEntity(uint32_t entityID);
	static void ClearSelectedEntity();
	static void SetEditorViewportSize(int width, int height);

	//engine state management
	void Coma();	//do not use recklessly, this will put the program into deadlock
	void Terminate();
};


#endif // Engine_HPP