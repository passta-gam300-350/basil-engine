/******************************************************************************/
/*!
\file   EDITOR_PROFILER.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2025/11/27
\brief This file contains the implementation for the EditorProfiler class, which
is responsible for displaying profiling information in the editor UI.


Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#include "Profiler/EDITOR_PROFILER.hpp"
#include "Service/EngineService.hpp"
#include "Profiler/profiler.hpp"

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <numeric>
#include <thread>
#include <cmath>

EditorProfiler::EditorProfiler()
	: m_FrameTimeHistory(FRAME_HISTORY_SIZE, 0.0f)
{
}

void EditorProfiler::Render(EngineContainerService& engineService)
{
	ImGui::Begin("Profiler");

	auto info = engineService.GetEngineInfo();

	// Update FPS/Delta display (with optional rate limiting for readability)
	double currentTime = glfwGetTime();
	if (!m_EnableRateLimiting || (currentTime - m_LastUpdateTime >= m_UpdateInterval)) {
		m_DisplayFPS = info.m_FPS;
		m_DisplayDeltaTime = info.m_DeltaTime;
		m_LastUpdateTime = currentTime;
	}

	// === HEADER SECTION ===
	// Calculate dynamic header height (12% of available height, clamped between 80-150px)
	float availableHeight = ImGui::GetContentRegionAvail().y;
	float headerHeight = availableHeight * 0.12f;
	headerHeight = std::max(80.0f, std::min(headerHeight, 150.0f));

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.15f, 1.0f));

	// FPS Display
	ImGui::Text("FPS: %.2f | Delta Time: %.3f ms", m_DisplayFPS, m_DisplayDeltaTime * 1000.0);
	ImGui::Separator();

	// Controls row
	ImGui::Columns(2, "##HeaderColumns", false);

	// Left column: Rate limiting
	ImGui::Checkbox("Rate Limit Updates", &m_EnableRateLimiting);
	if (m_EnableRateLimiting) {
		ImGui::SliderFloat("Update Interval", &m_UpdateInterval, 0.05f, 2.0f, "%.2f s");
	} else {
		ImGui::TextDisabled("Real-time updates enabled");
	}

	ImGui::NextColumn();

	// Right column: Global category filters
	ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Event Filters:");
	ImGui::Checkbox("Frame Events", &m_FilterFrameEvents);
	ImGui::SameLine();
	ImGui::Checkbox("System Events", &m_FilterSystemEvents);
	ImGui::SameLine();
	ImGui::Checkbox("Function Events", &m_FilterFunctionEvents);

	ImGui::Columns(1);
	ImGui::PopStyleColor();

	// Alert banner for recent spikes
	if (!m_RecentSpikes.empty() && m_EnableSpikeDetection) {
		const SpikeInfo& latestSpike = m_RecentSpikes.back();
		double timeSinceSpike = glfwGetTime() - latestSpike.timestamp;

		// Show warning if spike was within last 3 seconds
		if (timeSinceSpike < 3.0) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.0f, 0.0f, 0.5f));

			if (latestSpike.systemName.empty()) {
				ImGui::Text("! FRAME SPIKE DETECTED! %.2f ms (%.1fx average)", latestSpike.value, latestSpike.multiplier);
			} else {
				ImGui::Text("! SYSTEM SPIKE: %s - %.2f ms (%.1fx average)", latestSpike.systemName.c_str(), latestSpike.value, latestSpike.multiplier);
			}
			ImGui::PopStyleColor(2);
		}
	}

	ImGui::Separator();

	// === TABS FOR ENGINE VS EDITOR ===
	static int activeTab = 0; // 0 = Engine, 1 = Editor
	if (ImGui::BeginTabBar("ProfilerTabs", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem("Engine Thread"))
		{
			activeTab = 0;
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Editor Thread"))
		{
			activeTab = 1;
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	// Get data from the appropriate profiler
	Profiler& activeProfiler = (activeTab == 0) ? Profiler::instance() : GetEditorProfiler();
	auto events = activeProfiler.getEventCurrentFrame();
	auto last = activeProfiler.Get_Last_Frame();

	const char* threadName = (activeTab == 0) ? "Engine" : "Editor";

	// Update frame time history (circular buffer)
	if (last.frameMs > 0.0) {
		m_FrameTimeHistory[m_FrameHistoryIndex] = static_cast<float>(last.frameMs);
		m_FrameHistoryIndex = (m_FrameHistoryIndex + 1) % m_FrameTimeHistory.size();

		// Spike detection for frame time
		if (m_EnableSpikeDetection) {
			// Calculate average frame time (exclude zeros)
			double sumFrameTime = 0.0;
			int validFrames = 0;
			for (float ft : m_FrameTimeHistory) {
				if (ft > 0.0f) {
					sumFrameTime += ft;
					validFrames++;
				}
			}
			if (validFrames > 10) {  // Need enough samples
				double avgFrameTime = sumFrameTime / validFrames;
				double multiplier = last.frameMs / avgFrameTime;

				// Detect spike: must exceed BOTH relative threshold AND absolute minimum
				if (multiplier > m_SpikeThresholdMultiplier && last.frameMs > m_MinSpikeValueMs) {
					SpikeInfo spike;
					spike.frameIndex = last.frameIndex;
					spike.systemName = "";  // Empty = frame spike
					spike.value = last.frameMs;
					spike.average = avgFrameTime;
					spike.multiplier = multiplier;
					spike.timestamp = glfwGetTime();

					m_RecentSpikes.push_back(spike);
					if (m_RecentSpikes.size() > MAX_SPIKE_HISTORY) {
						m_RecentSpikes.erase(m_RecentSpikes.begin());
					}
				}
			}
		}
	}

	// Frame time graph
	ImGui::Text("%s Thread - Frame Time History (last %zu frames):", threadName, FRAME_HISTORY_SIZE);

	// Calculate min/max for auto-scaling
	float minFrameTime = *std::min_element(m_FrameTimeHistory.begin(), m_FrameTimeHistory.end());
	float maxFrameTime = *std::max_element(m_FrameTimeHistory.begin(), m_FrameTimeHistory.end());

	// Ensure minimum range for visibility
	if (maxFrameTime < 20.0f) maxFrameTime = 20.0f;

	// Draw graph with target line overlay
	char overlay[64];
	if (last.frameMs > TARGET_60FPS) {
		sprintf_s(overlay, "%.2f ms (%.0f FPS) - OVER 60 FPS BUDGET!", last.frameMs, 1000.0 / last.frameMs);
	} else {
		sprintf_s(overlay, "%.2f ms (%.0f FPS)", last.frameMs, 1000.0 / last.frameMs);
	}

	ImGui::PlotLines("##FrameTimeGraph", m_FrameTimeHistory.data(), static_cast<int>(m_FrameTimeHistory.size()),
	                 static_cast<int>(m_FrameHistoryIndex), overlay, minFrameTime, maxFrameTime, ImVec2(0, 100));

	// Draw spike markers on the graph
	if (!m_RecentSpikes.empty() && m_EnableSpikeDetection) {
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 graphMin = ImGui::GetItemRectMin();
		ImVec2 graphMax = ImGui::GetItemRectMax();
		float graphWidth = graphMax.x - graphMin.x;
		float graphHeight = graphMax.y - graphMin.y;

		for (const SpikeInfo& spike : m_RecentSpikes) {
			if (spike.systemName.empty()) {  // Only mark frame spikes on graph
				// Calculate relative position in circular buffer
				// Find how many frames ago this spike was
				int framesAgo = static_cast<int>(last.frameIndex - spike.frameIndex);
				if (framesAgo >= 0 && framesAgo < static_cast<int>(FRAME_HISTORY_SIZE)) {
					// Position in graph (from right to left)
					float normalizedX = 1.0f - (framesAgo / static_cast<float>(FRAME_HISTORY_SIZE));
					float x = graphMin.x + normalizedX * graphWidth;

					// Draw vertical red line at spike position
					drawList->AddLine(
						ImVec2(x, graphMin.y),
						ImVec2(x, graphMax.y),
						IM_COL32(255, 0, 0, 180),  // Semi-transparent red
						2.0f
					);
				}
			}
		}
	}

	// Target frame time indicators
	ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Target 60 FPS: %.2f ms", TARGET_60FPS);
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Target 30 FPS: %.2f ms", TARGET_30FPS);
	ImGui::SameLine();
	if (!m_RecentSpikes.empty()) {
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "| Red lines = Spikes");
	}

	ImGui::Separator();

	// Update system history (always runs - don't lose data!)
	for (auto& kv : last.systemMs) {
		auto& history = m_SystemHistory[kv.first];
		history.push_back(kv.second);
		if (history.size() > STAT_WINDOW) {
			history.erase(history.begin());
		}
	}

	// Recalculate stats only when rate limit allows (or if disabled)
	if (!m_EnableRateLimiting || (currentTime - m_LastUpdateTime >= m_UpdateInterval)) {
		// Calculate total time
		double totalMs = 0.0;
		for (auto& kv : last.systemMs) {
			totalMs += kv.second;
		}

		// Recalculate stats for all systems
		for (auto& kv : last.systemMs) {
			const std::string& systemName = kv.first;
			double sysMs = kv.second;
			double pct = (totalMs > 0.0) ? (sysMs / totalMs) * 100.0 : 0.0;

			auto& history = m_SystemHistory[systemName];
			if (!history.empty()) {
				double minMs = *std::min_element(history.begin(), history.end());
				double maxMs = *std::max_element(history.begin(), history.end());
				double avgMs = std::accumulate(history.begin(), history.end(), 0.0) / history.size();

				// Spike detection for systems: must exceed BOTH relative threshold AND absolute minimum
				if (m_EnableSpikeDetection && history.size() > 5 && avgMs > 0.0) {
					double multiplier = sysMs / avgMs;
					if (multiplier > m_SpikeThresholdMultiplier && sysMs > m_MinSpikeValueMs) {
						// Check if we already recorded this spike recently (avoid duplicates)
						bool alreadyRecorded = false;
						for (auto& recentSpike : m_RecentSpikes) {
							if (recentSpike.systemName == systemName &&
							    recentSpike.frameIndex == last.frameIndex) {
								alreadyRecorded = true;
								break;
							}
						}

						if (!alreadyRecorded) {
							SpikeInfo spike;
							spike.frameIndex = last.frameIndex;
							spike.systemName = systemName;
							spike.value = sysMs;
							spike.average = avgMs;
							spike.multiplier = multiplier;
							spike.timestamp = glfwGetTime();

							m_RecentSpikes.push_back(spike);
							if (m_RecentSpikes.size() > MAX_SPIKE_HISTORY) {
								m_RecentSpikes.erase(m_RecentSpikes.begin());
							}
						}
					}
				}

				// Color-code current value based on severity (enhanced with spike detection)
				ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White default

				// Check for spike: must exceed BOTH relative threshold AND absolute minimum
				bool isSpike = (avgMs > 0.0 &&
				                sysMs / avgMs > m_SpikeThresholdMultiplier &&
				                sysMs > m_MinSpikeValueMs);

				if (isSpike) {
					color = ImVec4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta for spike!
				} else if (pct > 50.0) {
					color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red if >50%
				} else if (pct > 30.0) {
					color = ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange if >30%
				}

				// Cache the calculated stats
				SystemStats& stats = m_CachedSystemStats[systemName];
				stats.current = sysMs;
				stats.min = minMs;
				stats.max = maxMs;
				stats.avg = avgMs;
				stats.percent = pct;
				stats.color = color;
			}
		}
	}

	// Display systems with statistics (always renders using cached values)
	ImGui::Text("System Performance (last %zu frames):", STAT_WINDOW);
	ImGui::Columns(6, "SystemStats");
	ImGui::Separator();
	ImGui::Text("System"); ImGui::NextColumn();
	ImGui::Text("Current"); ImGui::NextColumn();
	ImGui::Text("Min"); ImGui::NextColumn();
	ImGui::Text("Max"); ImGui::NextColumn();
	ImGui::Text("Avg"); ImGui::NextColumn();
	ImGui::Text("%%"); ImGui::NextColumn();
	ImGui::Separator();

	// Render from cached stats
	for (const auto& kv : m_CachedSystemStats) {
		const std::string& systemName = kv.first;
		const SystemStats& stats = kv.second;

		ImGui::TextColored(stats.color, "%s", systemName.c_str()); ImGui::NextColumn();
		ImGui::Text("%.2f ms", stats.current); ImGui::NextColumn();
		ImGui::Text("%.2f ms", stats.min); ImGui::NextColumn();
		ImGui::Text("%.2f ms", stats.max); ImGui::NextColumn();
		ImGui::Text("%.2f ms", stats.avg); ImGui::NextColumn();
		ImGui::TextColored(stats.color, "%.1f%%", stats.percent); ImGui::NextColumn();
	}

	ImGui::Columns(1);
	ImGui::Separator();

	// Per-System History Graphs
	if (ImGui::CollapsingHeader("Per-System Performance Graphs")) {
		ImGui::Checkbox("Show System Graphs", &m_ShowPerSystemGraphs);

		if (m_ShowPerSystemGraphs) {
			ImGui::Separator();

			// Render a graph for each system
			for (const auto& kv : m_SystemHistory) {
				const std::string& systemName = kv.first;
				const std::vector<double>& history = kv.second;

				if (history.empty()) continue;

				// Convert to float array for ImGui::PlotLines
				std::vector<float> historyFloat(history.size());
				for (size_t i = 0; i < history.size(); ++i) {
					historyFloat[i] = static_cast<float>(history[i]);
				}

				// Calculate stats for this system
				double minMs = *std::min_element(history.begin(), history.end());
				double maxMs = *std::max_element(history.begin(), history.end());
				double avgMs = std::accumulate(history.begin(), history.end(), 0.0) / history.size();
				double currentMs = history.back();

				// Color-code system name based on current performance
				ImVec4 nameColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				if (currentMs > avgMs * m_SpikeThresholdMultiplier && currentMs > m_MinSpikeValueMs) {
					nameColor = ImVec4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta for spike
				} else if (currentMs > avgMs * 1.5f) {
					nameColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange for elevated
				} else if (currentMs > avgMs * 1.2f) {
					nameColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow for slightly high
				} else {
					nameColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for normal
				}

				// Display system name with color
				ImGui::TextColored(nameColor, "%s", systemName.c_str());

				// Create overlay text with current stats
				char overlay[128];
				sprintf_s(overlay, "Current: %.2f ms | Avg: %.2f ms | Min: %.2f ms | Max: %.2f ms",
				          currentMs, avgMs, minMs, maxMs);

				// Draw the graph
				ImGui::PlotLines(
					("##Graph_" + systemName).c_str(),
					historyFloat.data(),
					static_cast<int>(historyFloat.size()),
					0,
					overlay,
					0.0f,                        // Min Y (auto-scale from 0)
					static_cast<float>(maxMs * 1.2f),  // Max Y (20% padding above max)
					ImVec2(0, 60)                // Size (auto width, 60px height)
				);

				ImGui::Separator();
			}
		} else {
			ImGui::TextDisabled("Enable to see individual performance graphs for each system");
		}
	}

	ImGui::Separator();

	// Function-Level Profiling Table
	if (ImGui::CollapsingHeader("Function Profiling")) {
		ImGui::Checkbox("Show Function Table", &m_ShowFunctionTable);

		if (m_ShowFunctionTable) {
			if (!m_FilterFunctionEvents) {
				ImGui::Separator();
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Function Events are filtered out!");
				ImGui::TextDisabled("Enable 'Function Events' filter in header to see data");
			} else {
				ImGui::Separator();

				// Aggregate function call data from current frame events
				struct FunctionStats {
					std::string name;
					double totalMs;
					int callCount;
					double avgMs;
					double percentOfFrame;
				};

				std::unordered_map<std::string, FunctionStats> functionData;
				double totalFrameMs = last.frameMs;

				// Parse events and aggregate by function name (respecting global filter)
				for (const Event& event : events) {
					if (event.category == Category::Function) {
						double durationMs = event.duration_ns / 1e6;
						std::string funcName(event.name);

						auto& stats = functionData[funcName];
						stats.name = funcName;
						stats.totalMs += durationMs;
						stats.callCount++;
					}
				}

				// Calculate averages and percentages
				for (auto& kv : functionData) {
					FunctionStats& stats = kv.second;
					stats.avgMs = stats.totalMs / stats.callCount;
					stats.percentOfFrame = (totalFrameMs > 0.0) ? (stats.totalMs / totalFrameMs) * 100.0 : 0.0;
				}

				// Convert to vector for sorting
				std::vector<FunctionStats> sortedFunctions;
				sortedFunctions.reserve(functionData.size());
				for (const auto& kv : functionData) {
					sortedFunctions.push_back(kv.second);
				}

				// Display table
				if (sortedFunctions.empty()) {
					ImGui::TextDisabled("No function-level profiling data available");
					ImGui::TextDisabled("Use PF_FUNC() or PF_SCOPE() macros to profile functions");
				} else {
					ImGui::Text("Profiled %zu unique functions (%zu total calls)", sortedFunctions.size(), events.size());
					ImGui::Separator();

					// Sortable table
					if (ImGui::BeginTable("FunctionTable", 5,
					                      ImGuiTableFlags_Sortable |
					                      ImGuiTableFlags_Borders |
					                      ImGuiTableFlags_RowBg |
					                      ImGuiTableFlags_ScrollY,
					                      ImVec2(0, 300))) {
					// Setup columns
					ImGui::TableSetupColumn("Function", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn("Total (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
					ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, 60.0f);
					ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
					ImGui::TableSetupColumn("% Frame", ImGuiTableColumnFlags_WidthFixed, 70.0f);
					ImGui::TableSetupScrollFreeze(0, 1); // Freeze header row
					ImGui::TableHeadersRow();

					// Handle sorting
					if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
						if (sortSpecs->SpecsDirty) {
							// Sort based on clicked column
							if (sortSpecs->SpecsCount > 0) {
								const ImGuiTableColumnSortSpecs& spec = sortSpecs->Specs[0];
								bool ascending = (spec.SortDirection == ImGuiSortDirection_Ascending);

								std::sort(sortedFunctions.begin(), sortedFunctions.end(),
								          [&spec, ascending](const FunctionStats& a, const FunctionStats& b) {
									switch (spec.ColumnIndex) {
										case 0: return ascending ? (a.name < b.name) : (a.name > b.name);
										case 1: return ascending ? (a.totalMs < b.totalMs) : (a.totalMs > b.totalMs);
										case 2: return ascending ? (a.callCount < b.callCount) : (a.callCount > b.callCount);
										case 3: return ascending ? (a.avgMs < b.avgMs) : (a.avgMs > b.avgMs);
										case 4: return ascending ? (a.percentOfFrame < b.percentOfFrame) : (a.percentOfFrame > b.percentOfFrame);
										default: return false;
									}
								});
							}
							sortSpecs->SpecsDirty = false;
						}
					}

					// Render rows
					for (const FunctionStats& func : sortedFunctions) {
						ImGui::TableNextRow();

						// Color code based on performance impact
						ImVec4 rowColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
						if (func.percentOfFrame > 10.0) {
							rowColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red - significant impact
						} else if (func.percentOfFrame > 5.0) {
							rowColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange - moderate impact
						} else if (func.percentOfFrame > 2.0) {
							rowColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow - noticeable
						}

						ImGui::TableNextColumn();
						ImGui::TextColored(rowColor, "%s", func.name.c_str());

						ImGui::TableNextColumn();
						ImGui::Text("%.3f", func.totalMs);

						ImGui::TableNextColumn();
						ImGui::Text("%d", func.callCount);

						ImGui::TableNextColumn();
						ImGui::Text("%.3f", func.avgMs);

						ImGui::TableNextColumn();
						ImGui::TextColored(rowColor, "%.2f%%", func.percentOfFrame);
					}

						ImGui::EndTable();
					}
				}
			}
		} else {
			ImGui::TextDisabled("Enable to see detailed function-level profiling statistics");
		}
	}

	ImGui::Separator();

	// Thread Visualization
	if (ImGui::CollapsingHeader("Thread Visualization")) {
		ImGui::Checkbox("Show Thread Activity", &m_ShowThreadVisualization);

		if (m_ShowThreadVisualization) {
			ImGui::Separator();
			ImGui::TextDisabled("Respects global event filters from header section");

			// Group events by thread ID (respecting global category filters)
			std::unordered_map<std::thread::id, std::vector<const Event*>> eventsByThread;
			for (const Event& event : events) {
				// Apply global category filters
				bool shouldShow = false;
				if (event.category == Category::Frame && m_FilterFrameEvents) shouldShow = true;
				if (event.category == Category::System && m_FilterSystemEvents) shouldShow = true;
				if (event.category == Category::Function && m_FilterFunctionEvents) shouldShow = true;

				if (shouldShow) {
					eventsByThread[event.tid].push_back(&event);
				}
			}

			if (eventsByThread.empty()) {
				ImGui::TextDisabled("No thread data available");
			} else {
				ImGui::Text("Active threads: %zu", eventsByThread.size());
				ImGui::Separator();

				// Assign colors to threads (hash-based for consistency)
				auto getThreadColor = [](const std::thread::id& tid) -> ImVec4 {
					size_t hash = std::hash<std::thread::id>{}(tid);
					float hue = (hash % 360) / 360.0f;
					// Convert HSV to RGB (simplified - pastel colors)
					float r = 0.5f + 0.5f * cosf((hue * 6.28f));
					float g = 0.5f + 0.5f * cosf((hue * 6.28f) + 2.09f);
					float b = 0.5f + 0.5f * cosf((hue * 6.28f) + 4.19f);
					return ImVec4(r, g, b, 1.0f);
				};

				// Display each thread's activity
				int threadIndex = 0;
				for (const auto& kv : eventsByThread) {
					const std::thread::id& tid = kv.first;
					const std::vector<const Event*>& threadEvents = kv.second;

					ImVec4 threadColor = getThreadColor(tid);

					// Thread header with color indicator
					char threadLabel[128];
					sprintf_s(threadLabel, "Thread #%d (%zu events)", threadIndex, threadEvents.size());

					ImGui::PushStyleColor(ImGuiCol_Header, threadColor);
					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(threadColor.x * 1.2f, threadColor.y * 1.2f, threadColor.z * 1.2f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(threadColor.x * 0.8f, threadColor.y * 0.8f, threadColor.z * 0.8f, 1.0f));

					if (ImGui::CollapsingHeader(threadLabel)) {
						ImGui::PopStyleColor(3);

						// Calculate thread statistics
						double totalThreadMs = 0.0;
						std::unordered_map<std::string, int> eventCounts;
						for (const Event* e : threadEvents) {
							totalThreadMs += e->duration_ns / 1e6;
							eventCounts[e->name]++;
						}

						// Display thread summary
						ImGui::Indent();
						ImGui::Text("Total time: %.2f ms (%.1f%% of frame)", totalThreadMs,
						            (last.frameMs > 0.0) ? (totalThreadMs / last.frameMs * 100.0) : 0.0);
						ImGui::Separator();

						// Display event breakdown for this thread
						if (ImGui::BeginTable(("ThreadTable_" + std::to_string(threadIndex)).c_str(), 4,
						                      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg,
						                      ImVec2(0, 200))) {
							ImGui::TableSetupColumn("Event");
							ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 60.0f);
							ImGui::TableSetupColumn("Total (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
							ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 80.0f);
							ImGui::TableHeadersRow();

							// Aggregate events by name
							std::unordered_map<std::string, std::pair<int, double>> aggregated;
							std::unordered_map<std::string, Category> categories;
							for (const Event* e : threadEvents) {
								aggregated[e->name].first++;  // count
								aggregated[e->name].second += e->duration_ns / 1e6;  // total ms
								categories[e->name] = e->category;
							}

							// Sort by total time
							std::vector<std::pair<std::string, std::pair<int, double>>> sorted(
								aggregated.begin(), aggregated.end()
							);
							std::sort(sorted.begin(), sorted.end(),
							          [](const auto& a, const auto& b) {
								return a.second.second > b.second.second;  // Sort by total time descending
							});

							// Display rows
							for (const auto& item : sorted) {
								const std::string& name = item.first;
								int count = item.second.first;
								double totalMs = item.second.second;
								Category cat = categories[name];

								ImGui::TableNextRow();

								ImGui::TableNextColumn();
								ImGui::Text("%s", name.c_str());

								ImGui::TableNextColumn();
								ImGui::Text("%d", count);

								ImGui::TableNextColumn();
								ImGui::Text("%.3f", totalMs);

								ImGui::TableNextColumn();
								const char* catName = "";
								ImVec4 catColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
								switch (cat) {
									case Category::Frame:
										catName = "Frame";
										catColor = ImVec4(0.5f, 0.5f, 1.0f, 1.0f);
										break;
									case Category::System:
										catName = "System";
										catColor = ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
										break;
									case Category::Function:
										catName = "Function";
										catColor = ImVec4(1.0f, 1.0f, 0.5f, 1.0f);
										break;
								}
								ImGui::TextColored(catColor, "%s", catName);
							}

							ImGui::EndTable();
						}
						ImGui::Unindent();
					} else {
						ImGui::PopStyleColor(3);
					}

					threadIndex++;
				}
			}
		} else {
			ImGui::TextDisabled("Enable to see which events are running on which threads");
		}
	}

	ImGui::Separator();

	// Multi-Frame Statistics
	if (ImGui::CollapsingHeader("Multi-Frame Statistics")) {
		ImGui::Checkbox("Show Extended Stats", &m_ShowMultiFrameStats);

		if (m_ShowMultiFrameStats) {
			ImGui::Separator();

			// Get all frame history
			auto allFrames = activeProfiler.getFramesSnapshot();

			if (allFrames.empty()) {
				ImGui::TextDisabled("No frame history available yet");
			} else {
				// Extract frame times
				std::vector<double> frameTimes;
				frameTimes.reserve(allFrames.size());
				for (const auto& frame : allFrames) {
					frameTimes.push_back(frame.frameMs);
				}

				// Calculate statistics
				double minFrame = *std::min_element(frameTimes.begin(), frameTimes.end());
				double maxFrame = *std::max_element(frameTimes.begin(), frameTimes.end());
				double avgFrame = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0) / frameTimes.size();

				// Calculate percentiles
				std::vector<double> sortedFrames = frameTimes;
				std::sort(sortedFrames.begin(), sortedFrames.end());

				auto getPercentile = [&sortedFrames](double percentile) -> double {
					size_t index = static_cast<size_t>(sortedFrames.size() * percentile);
					if (index >= sortedFrames.size()) index = sortedFrames.size() - 1;
					return sortedFrames[index];
				};

				double p50 = getPercentile(0.50);  // Median
				double p95 = getPercentile(0.95);
				double p99 = getPercentile(0.99);

				// Count frames over budget
				int framesOver60 = 0;
				int framesOver30 = 0;
				for (double ft : frameTimes) {
					if (ft > TARGET_60FPS) framesOver60++;
					if (ft > TARGET_30FPS) framesOver30++;
				}

				// Display statistics
				ImGui::Text("Analyzing last %zu frames (%.1f seconds @ 60 FPS)",
				            allFrames.size(), allFrames.size() / 60.0);
				ImGui::Separator();

				// Frame time statistics
				ImGui::Columns(2, "StatsColumns");

				ImGui::Text("Frame Time Statistics:");
				ImGui::NextColumn();
				ImGui::NextColumn();

				ImGui::Text("  Min:");
				ImGui::NextColumn();
				ImGui::Text("%.2f ms (%.0f FPS)", minFrame, 1000.0 / minFrame);
				ImGui::NextColumn();

				ImGui::Text("  Max:");
				ImGui::NextColumn();
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "%.2f ms (%.0f FPS)", maxFrame, 1000.0 / maxFrame);
				ImGui::NextColumn();

				ImGui::Text("  Average:");
				ImGui::NextColumn();
				ImGui::Text("%.2f ms (%.0f FPS)", avgFrame, 1000.0 / avgFrame);
				ImGui::NextColumn();

				ImGui::Text("  Median (50th):");
				ImGui::NextColumn();
				ImGui::Text("%.2f ms", p50);
				ImGui::NextColumn();

				ImGui::Text("  95th Percentile:");
				ImGui::NextColumn();
				ImGui::Text("%.2f ms", p95);
				ImGui::NextColumn();

				ImGui::Text("  99th Percentile:");
				ImGui::NextColumn();
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%.2f ms", p99);
				ImGui::NextColumn();

				ImGui::Separator();
				ImGui::NextColumn();
				ImGui::NextColumn();

				ImGui::Text("Performance Budget:");
				ImGui::NextColumn();
				ImGui::NextColumn();

				ImGui::Text("  Frames over 60 FPS budget:");
				ImGui::NextColumn();
				double pct60 = (frameTimes.size() > 0) ? (framesOver60 / (double)frameTimes.size() * 100.0) : 0.0;
				ImVec4 color60 = (pct60 > 50.0) ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) :
				                 (pct60 > 25.0) ? ImVec4(1.0f, 0.5f, 0.0f, 1.0f) :
				                                  ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
				ImGui::TextColored(color60, "%d / %zu (%.1f%%)", framesOver60, frameTimes.size(), pct60);
				ImGui::NextColumn();

				ImGui::Text("  Frames over 30 FPS budget:");
				ImGui::NextColumn();
				double pct30 = (frameTimes.size() > 0) ? (framesOver30 / (double)frameTimes.size() * 100.0) : 0.0;
				ImVec4 color30 = (pct30 > 10.0) ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) :
				                 (pct30 > 5.0) ? ImVec4(1.0f, 0.5f, 0.0f, 1.0f) :
				                                 ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
				ImGui::TextColored(color30, "%d / %zu (%.1f%%)", framesOver30, frameTimes.size(), pct30);
				ImGui::NextColumn();

				ImGui::Columns(1);
			}
		} else {
			ImGui::TextDisabled("Enable to see comprehensive statistics across all frames");
		}
	}

	ImGui::Separator();

	// Call Count Analysis
	if (ImGui::CollapsingHeader("Call Count Analysis")) {
		ImGui::Checkbox("Show Call Frequency", &m_ShowCallCountAnalysis);

		if (m_ShowCallCountAnalysis) {
			ImGui::Separator();
			ImGui::TextDisabled("Respects global event filters from header section");

			// Aggregate call counts from current frame
			struct CallStats {
				std::string name;
				int callCount;
				double totalMs;
				double avgMs;
				double percentOfFrame;
			};

			std::unordered_map<std::string, CallStats> callData;
			double totalFrameMs = last.frameMs;

			// Parse all events (respecting global category filters)
			for (const Event& event : events) {
				// Apply global category filters
				bool shouldShow = false;
				if (event.category == Category::Frame && m_FilterFrameEvents) shouldShow = true;
				if (event.category == Category::System && m_FilterSystemEvents) shouldShow = true;
				if (event.category == Category::Function && m_FilterFunctionEvents) shouldShow = true;

				if (!shouldShow) continue;

				double durationMs = event.duration_ns / 1e6;
				std::string eventName(event.name);

				auto& stats = callData[eventName];
				stats.name = eventName;
				stats.callCount++;
				stats.totalMs += durationMs;
			}

			// Calculate averages
			for (auto& kv : callData) {
				CallStats& stats = kv.second;
				stats.avgMs = stats.totalMs / stats.callCount;
				stats.percentOfFrame = (totalFrameMs > 0.0) ? (stats.totalMs / totalFrameMs) * 100.0 : 0.0;
			}

			// Convert to vector and sort by call count
			std::vector<CallStats> sortedCalls;
			sortedCalls.reserve(callData.size());
			for (const auto& kv : callData) {
				sortedCalls.push_back(kv.second);
			}
			std::sort(sortedCalls.begin(), sortedCalls.end(),
			          [](const CallStats& a, const CallStats& b) {
				return a.callCount > b.callCount;  // Sort by call count descending
			});

			if (sortedCalls.empty()) {
				ImGui::TextDisabled("No call data available");
			} else {
				ImGui::Text("Event call frequency analysis (current frame)");
				ImGui::TextDisabled("Identify functions/systems called too frequently");
				ImGui::Separator();

				// Display sortable table
				if (ImGui::BeginTable("CallCountTable", 5,
				                      ImGuiTableFlags_Sortable |
				                      ImGuiTableFlags_Borders |
				                      ImGuiTableFlags_RowBg |
				                      ImGuiTableFlags_ScrollY,
				                      ImVec2(0, 300))) {
					ImGui::TableSetupColumn("Event Name", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn("Calls/Frame", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort, 90.0f);
					ImGui::TableSetupColumn("Total (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
					ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
					ImGui::TableSetupColumn("% Frame", ImGuiTableColumnFlags_WidthFixed, 70.0f);
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableHeadersRow();

					// Handle sorting
					if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
						if (sortSpecs->SpecsDirty) {
							if (sortSpecs->SpecsCount > 0) {
								const ImGuiTableColumnSortSpecs& spec = sortSpecs->Specs[0];
								bool ascending = (spec.SortDirection == ImGuiSortDirection_Ascending);

								std::sort(sortedCalls.begin(), sortedCalls.end(),
								          [&spec, ascending](const CallStats& a, const CallStats& b) {
									switch (spec.ColumnIndex) {
										case 0: return ascending ? (a.name < b.name) : (a.name > b.name);
										case 1: return ascending ? (a.callCount < b.callCount) : (a.callCount > b.callCount);
										case 2: return ascending ? (a.totalMs < b.totalMs) : (a.totalMs > b.totalMs);
										case 3: return ascending ? (a.avgMs < b.avgMs) : (a.avgMs > b.avgMs);
										case 4: return ascending ? (a.percentOfFrame < b.percentOfFrame) : (a.percentOfFrame > b.percentOfFrame);
										default: return false;
									}
								});
							}
							sortSpecs->SpecsDirty = false;
						}
					}

					// Render rows
					for (const CallStats& call : sortedCalls) {
						ImGui::TableNextRow();

						// Color code based on call frequency
						ImVec4 rowColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
						if (call.callCount > 1000) {
							rowColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red - very high
						} else if (call.callCount > 100) {
							rowColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange - high
						} else if (call.callCount > 10) {
							rowColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow - moderate
						}

						ImGui::TableNextColumn();
						ImGui::Text("%s", call.name.c_str());

						ImGui::TableNextColumn();
						ImGui::TextColored(rowColor, "%d", call.callCount);

						ImGui::TableNextColumn();
						ImGui::Text("%.3f", call.totalMs);

						ImGui::TableNextColumn();
						ImGui::Text("%.4f", call.avgMs);

						ImGui::TableNextColumn();
						ImGui::Text("%.2f%%", call.percentOfFrame);
					}

					ImGui::EndTable();
				}

				ImGui::Separator();
				ImGui::TextDisabled("Tip: Functions called >100 times/frame are optimization candidates");
			}
		} else {
			ImGui::TextDisabled("Enable to see which events are called most frequently");
		}
	}

	ImGui::Separator();

	// Performance Spike Detection
	if (ImGui::CollapsingHeader("Performance Spikes", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Enable Spike Detection", &m_EnableSpikeDetection);
		ImGui::SliderFloat("Multiplier Threshold", &m_SpikeThresholdMultiplier, 1.5f, 5.0f, "%.1fx average");
		ImGui::SliderFloat("Minimum Value Threshold", &m_MinSpikeValueMs, 0.01f, 2.0f, "%.2f ms");
		ImGui::TextDisabled("(Spikes must exceed BOTH thresholds to be detected)");

		if (m_RecentSpikes.empty()) {
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "No performance spikes detected! (%.1fx & >%.2fms)",
			                   m_SpikeThresholdMultiplier, m_MinSpikeValueMs);
		} else {
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Detected %zu spike(s) in last %zu frames:", m_RecentSpikes.size(), MAX_SPIKE_HISTORY);

			ImGui::Separator();

			// Display spikes in reverse order (most recent first)
			for (auto it = m_RecentSpikes.rbegin(); it != m_RecentSpikes.rend(); ++it) {
				const SpikeInfo& spike = *it;
				double timeAgo = glfwGetTime() - spike.timestamp;

				if (spike.systemName.empty()) {
					// Frame spike
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
					ImGui::Text("[FRAME SPIKE]");
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::Text("Frame #%zu: %.2f ms (%.1fx avg of %.2f ms) - %.1fs ago",
					            spike.frameIndex, spike.value, spike.multiplier, spike.average, timeAgo);
				} else {
					// System spike
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
					ImGui::Text("[SYSTEM SPIKE]");
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::Text("%s: %.2f ms (%.1fx avg of %.2f ms) - %.1fs ago",
					            spike.systemName.c_str(), spike.value, spike.multiplier, spike.average, timeAgo);
				}
			}

			ImGui::Separator();

			if (ImGui::Button("Clear Spike Log")) {
				m_RecentSpikes.clear();
			}
		}
	}

	ImGui::Separator();

	// Hierarchical Event Tree View
	if (ImGui::CollapsingHeader("Event Tree (Current Frame)", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("Slow Event Threshold (ms)", &m_SlowEventThresholdMs, 0.1f, 10.0f, "%.1f ms");
		ImGui::TextDisabled("Use global event filters in header to control visibility");

		ImGui::Separator();

		// Render the tree
		if (m_ShowEventTree && !events.empty()) {
			RenderEventTree(events);
		} else if (events.empty()) {
			ImGui::TextDisabled("No events recorded this frame");
		}
	}

	ImGui::End();
}

void EditorProfiler::RenderEventTree(const std::vector<Event>& events)
{
	// Track depth for tree hierarchy
	int currentDepth = -1;

	// Track if we're inside a collapsed node (to skip children)
	int skipUntilDepth = -1;

	for (size_t i = 0; i < events.size(); ++i) {
		const Event& event = events[i];

		// Filter by category (using global filters from header)
		bool shouldShow = false;
		if (event.category == Category::Frame && m_FilterFrameEvents) shouldShow = true;
		if (event.category == Category::System && m_FilterSystemEvents) shouldShow = true;
		if (event.category == Category::Function && m_FilterFunctionEvents) shouldShow = true;

		if (!shouldShow) continue;

		// If we're skipping children of a collapsed node, check if we've exited
		if (skipUntilDepth >= 0) {
			if (static_cast<int>(event.depth) > skipUntilDepth) {
				// Still inside collapsed node's children, skip this event
				continue;
			} else {
				// We've exited the collapsed node's children
				skipUntilDepth = -1;
			}
		}

		// Close any tree nodes we've exited (depth decreased)
		while (currentDepth >= static_cast<int>(event.depth)) {
			ImGui::TreePop();
			currentDepth--;
		}

		// Convert duration to milliseconds
		double durationMs = event.duration_ns / 1e6;

		// Check if next VISIBLE event is a child (accounting for filters)
		bool hasChildren = false;
		for (size_t j = i + 1; j < events.size(); ++j) {
			// Check if this event would be shown
			bool nextShouldShow = false;
			if (events[j].category == Category::Frame && m_FilterFrameEvents) nextShouldShow = true;
			if (events[j].category == Category::System && m_FilterSystemEvents) nextShouldShow = true;
			if (events[j].category == Category::Function && m_FilterFunctionEvents) nextShouldShow = true;

			if (!nextShouldShow) continue; // Skip filtered events

			// Check if this visible event is a child
			if (events[j].depth > event.depth) {
				hasChildren = true;
			}
			// Stop checking once we find a visible event (child or sibling)
			break;
		}

		// Color code based on duration
		ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White default
		if (durationMs > m_SlowEventThresholdMs * 5.0f) {
			color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red - very slow
		} else if (durationMs > m_SlowEventThresholdMs) {
			color = ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange - slow
		} else if (durationMs > m_SlowEventThresholdMs * 0.5f) {
			color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow - moderate
		}

		// Category indicator
		const char* categoryIcon = "";
		switch (event.category) {
			case Category::Frame: categoryIcon = "[F]"; break;
			case Category::System: categoryIcon = "[S]"; break;
			case Category::Function: categoryIcon = "[Fn]"; break;
		}

		// Build label
		char label[256];
		sprintf_s(label, "%s %s: %.3f ms", categoryIcon, event.name, durationMs);

		if (hasChildren) {
			// Node with children - make it expandable
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			bool isOpen = ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen);
			ImGui::PopStyleColor();

			if (isOpen) {
				// Track that we opened this node
				currentDepth = static_cast<int>(event.depth);
			} else {
				// Node is collapsed - mark to skip all its children
				skipUntilDepth = static_cast<int>(event.depth);
			}
		} else {
			// Leaf node - no children
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			ImGui::PopStyleColor();
		}
	}

	// Close any remaining open tree nodes
	while (currentDepth >= 0) {
		//ImGui::TreePop();
		currentDepth--;
	}
}
