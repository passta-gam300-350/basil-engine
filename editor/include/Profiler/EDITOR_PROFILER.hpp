/******************************************************************************/
/*!
\file   EDITOR_PROFILER.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2025/11/27
\brief This file contains the declaration for the EditorProfiler class, which
is responsible for displaying profiling information in the editor UI.


Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#ifndef EDITOR_PROFILER_HPP
#define EDITOR_PROFILER_HPP

#include <vector>
#include <unordered_map>
#include <string>
#include <imgui.h>  // For ImVec4 in SystemStats

// Forward declarations
struct EngineContainerService;

class EditorProfiler {
public:
	EditorProfiler();

	// Render the profiler UI window
	void Render(EngineContainerService& engineService);

private:
	// Helper methods
	void RenderEventTree(const std::vector<struct Event>& events);
	// FPS display throttling
	double m_LastUpdateTime = 0.0;
	double m_DisplayFPS = 0.0;
	double m_DisplayDeltaTime = 0.0;
	float m_UpdateInterval = 0.25f;  // Update every 0.25 seconds
	bool m_EnableRateLimiting = true;  // Toggle for rate limiting display updates

	// Frame time history (circular buffer for graph)
	std::vector<float> m_FrameTimeHistory;
	size_t m_FrameHistoryIndex = 0;
	static constexpr size_t FRAME_HISTORY_SIZE = 120;

	// System statistics (min/max/avg tracking)
	std::unordered_map<std::string, std::vector<double>> m_SystemHistory;
	static constexpr size_t STAT_WINDOW = 60;  // Track last 60 frames

	// Cached system stats for rate-limited display
	struct SystemStats {
		double current;
		double min;
		double max;
		double avg;
		double percent;
		ImVec4 color;
	};
	std::unordered_map<std::string, SystemStats> m_CachedSystemStats;

	// Frame time targets
	static constexpr float TARGET_60FPS = 16.67f;
	static constexpr float TARGET_30FPS = 33.33f;

	// Global category filters
	bool m_FilterFrameEvents = false;    // Usually too verbose
	bool m_FilterSystemEvents = true;
	bool m_FilterFunctionEvents = true;

	// Event tree view options
	bool m_ShowEventTree = true;
	float m_SlowEventThresholdMs = 1.0f;  // Highlight events slower than this

	// Per-system graphs
	bool m_ShowPerSystemGraphs = false;

	// Function-level profiling
	bool m_ShowFunctionTable = false;

	// Thread visualization
	bool m_ShowThreadVisualization = false;

	// Multi-frame statistics
	bool m_ShowMultiFrameStats = false;

	// Call count analysis
	bool m_ShowCallCountAnalysis = false;

	// Spike detection
	struct SpikeInfo {
		size_t frameIndex;
		std::string systemName;  // Empty for frame spikes
		double value;
		double average;
		double multiplier;
		double timestamp;  // glfwGetTime() when detected
	};
	std::vector<SpikeInfo> m_RecentSpikes;
	static constexpr size_t MAX_SPIKE_HISTORY = 20;
	float m_SpikeThresholdMultiplier = 2.0f;  // Spike = value > avg * multiplier
	float m_MinSpikeValueMs = 0.5f;            // Ignore spikes below this absolute value (ms)
	bool m_EnableSpikeDetection = true;
};

#endif // EDITOR_PROFILER_HPP

/* Usage Example for the time Profiling
Example 1: Automatic Nesting in a System

  void RenderSystem::Update() {
	  PF_SYSTEM("RenderSystem");  // depth = 0, then t_scopeDepth becomes 1

	  // Any profiling here is automatically at depth 1
	  {
		  PF_SCOPE("ShadowPass");  // depth = 1, then t_scopeDepth becomes 2
		  DrawShadows();  // If this has PF_FUNC(), it's depth 2
	  }  // ShadowPass destructor: t_scopeDepth back to 1

	  {
		  PF_SCOPE("MainPass");  // depth = 1, then t_scopeDepth becomes 2
		  DrawMain();
	  }  // MainPass destructor: t_scopeDepth back to 1

  }  // RenderSystem destructor: t_scopeDepth back to 0
*/