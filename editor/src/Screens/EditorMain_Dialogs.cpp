#include "Screens/EditorMain.hpp"

#include "Profiler/profiler.hpp"
#include "Bindings/MANAGED_CONSOLE.hpp"
#include "Scene/Scene.hpp"
#include "Render/Render.h"
#include "Physics/Physics_System.h"
#include "GLFW/glfw3.h"
#include <algorithm>
#include <ranges>
#include <numeric>
#include <unordered_map>
#include "Manager/BuildManager.hpp"
#include "Editor.hpp"

void EditorMain::Render_Profiler()
{
	PF_EDITOR_SCOPE("Render_Profiler");
	m_Profiler.Render(engineService);
}

void EditorMain::Render_Console()
{
	PF_EDITOR_SCOPE("Render_Console");

	// OPTIMIZATION: Check if window is visible before doing expensive work
	if (!ImGui::Begin("Console", &showConsole)) {
		ImGui::End();
		return;
	}
	
	ImGui::PushID("ConsoleClearBtn");
	bool clearConsole = ImGui::Button("Clear Console");
	ImGui::PopID();
	static unsigned char filterType = 0x07; // All types enabled by default (Info, Warning, Error)
	auto consoleMessages = ManagedConsole::TryGetMessages(filterType);
	if (filterType & 0x01) {
		//Dark green color for info and ux
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.0f, 1.0f)); // Green
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.6f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));

	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f)); // Greyed out
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	}
	ImGui::SameLine();
	ImGui::PushID("InfoFilterBtn");
	if (ImGui::Button("Info")) {
		// Toggle Info filter
		filterType ^= 0x01;
	}
	ImGui::PopID();
	ImGui::PopStyleColor(3);

	if (filterType & 0x02) {
		//Dark yellow color for warning
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.0f, 1.0f)); // Yellow
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.7f, 0.0f, 1.0f));
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f)); // Greyed out
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	}
	ImGui::SameLine();
	ImGui::PushID("WarningFilterBtn");
	if (ImGui::Button("Warning")) {
		// Toggle Warning filter
		filterType ^= 0x02;
	}
	ImGui::PopID();
	ImGui::PopStyleColor(3);

	if (filterType & 0x04) {
		//Dark red color for error
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.0f, 0.0f, 1.0f)); // Red
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.0f, 0.0f, 1.0f));
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f)); // Greyed out
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	}
	ImGui::SameLine();
	ImGui::PushID("ErrorFilterBtn");
	if (ImGui::Button("Error")) {
		// Toggle Error filter
		filterType ^= 0x04;
	}
	ImGui::PopID();
	ImGui::PopStyleColor(3);


	// Example log messages

	if (!consoleMessages.empty())
	{
		ImGui::BeginChild("ConsoleScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
		for (const auto& msg : consoleMessages) {
			switch (msg.second.type) {
			case ManagedConsole::Message::Type::Info:
				if (msg.first > 1)
					ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[INFO]: %s [%d]", msg.second.content.c_str(), msg.first);
				else
					ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[INFO]: %s", msg.second.content.c_str());
				break;
			case ManagedConsole::Message::Type::Warning:
				//Yellow color
				if (msg.first > 1)
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[WARNING]: %s [%d]", msg.second.content.c_str(), msg.first);
				else
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[WARNING]: %s", msg.second.content.c_str());
				break;
			case ManagedConsole::Message::Type::Error:
				//Red color
				if (msg.first > 1)
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[ERROR]: %s [%d]", msg.second.content.c_str(), msg.first);
				else
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[ERROR]: %s", msg.second.content.c_str());
				break;
			default:
				ImGui::Text("[UNKNOWN]: %s", msg.second.content.c_str());
				break;
			}
		}

		if (clearConsole)
		{
			ManagedConsole::Shutdown();
		}
		ImGui::EndChild();
	}

	ImGui::End();
}

void EditorMain::Render_EngineConsole()
{
	PF_EDITOR_SCOPE("Render_EngineConsole");

	// Initialize the engine console sink on first call
	InitializeEngineConsoleSink();

	// OPTIMIZATION: Check if window is visible before doing expensive work
	if (!ImGui::Begin("Engine Console", &showEngineConsole)) {
		ImGui::End();
		return;
	}

	ImGui::PushID("EngineConsoleClearBtn");
	bool clearConsole = ImGui::Button("Clear Console");
	ImGui::PopID();

	// Transfer new messages from engine buffer (short critical section!)
	auto newMessages = EngineConsole::TransferMessages();

	// Deduplicate into local storage (NO LOCK - only editor thread accesses this!)
	for (auto& newMsg : newMessages) {
		auto it = std::find_if(m_EngineConsoleLocalMessages.begin(), m_EngineConsoleLocalMessages.end(),
			[&newMsg](const std::pair<int, EngineConsole::Message>& msg) {
				return msg.second.type == newMsg.type && msg.second.content == newMsg.content;
			});
		if (it == m_EngineConsoleLocalMessages.end()) {
			m_EngineConsoleLocalMessages.push_back({ 1, std::move(newMsg) });
		}
		else {
			it->first += 1; // Increment count
		}
	}

	// Filter messages for display (reading from local storage, no lock!)
	auto filteredMessages = m_EngineConsoleLocalMessages | std::views::filter([this](const std::pair<int, EngineConsole::Message>& msg) {
		switch (msg.second.type) {
		case EngineConsole::Message::Type::Trace:
			return (m_EngineConsoleFilterType & 0x01) != 0;
		case EngineConsole::Message::Type::Debug:
			return (m_EngineConsoleFilterType & 0x02) != 0;
		case EngineConsole::Message::Type::Info:
			return (m_EngineConsoleFilterType & 0x04) != 0;
		case EngineConsole::Message::Type::Warn:
			return (m_EngineConsoleFilterType & 0x08) != 0;
		case EngineConsole::Message::Type::Error:
			return (m_EngineConsoleFilterType & 0x10) != 0;
		case EngineConsole::Message::Type::Critical:
			return (m_EngineConsoleFilterType & 0x20) != 0;
		case EngineConsole::Message::Type::Undefined:
			return (m_EngineConsoleFilterType & 0x40) != 0;
		default:
			return false;
		}
	});

	// Trace filter button
	if (m_EngineConsoleFilterType & 0x01) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Light gray
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f)); // Greyed out
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	}
	ImGui::SameLine();
	ImGui::PushID("EngineTraceFilterBtn");
	if (ImGui::Button("Trace")) {
		m_EngineConsoleFilterType ^= 0x01;
	}
	ImGui::PopID();
	ImGui::PopStyleColor(3);

	// Debug filter button
	if (m_EngineConsoleFilterType & 0x02) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.5f, 1.0f)); // Cyan
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.6f, 0.6f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.7f, 0.7f, 1.0f));
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f)); // Greyed out
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	}
	ImGui::SameLine();
	ImGui::PushID("EngineDebugFilterBtn");
	if (ImGui::Button("Debug")) {
		m_EngineConsoleFilterType ^= 0x02;
	}
	ImGui::PopID();
	ImGui::PopStyleColor(3);

	// Info filter button
	if (m_EngineConsoleFilterType & 0x04) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.0f, 1.0f)); // Green
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.6f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f)); // Greyed out
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	}
	ImGui::SameLine();
	ImGui::PushID("EngineInfoFilterBtn");
	if (ImGui::Button("Info")) {
		m_EngineConsoleFilterType ^= 0x04;
	}
	ImGui::PopID();
	ImGui::PopStyleColor(3);

	// Warn filter button
	if (m_EngineConsoleFilterType & 0x08) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.0f, 1.0f)); // Yellow
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.7f, 0.0f, 1.0f));
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f)); // Greyed out
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	}
	ImGui::SameLine();
	ImGui::PushID("EngineWarnFilterBtn");
	if (ImGui::Button("Warn")) {
		m_EngineConsoleFilterType ^= 0x08;
	}
	ImGui::PopID();
	ImGui::PopStyleColor(3);

	// Error filter button
	if (m_EngineConsoleFilterType & 0x10) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.0f, 0.0f, 1.0f)); // Red
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.0f, 0.0f, 1.0f));
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f)); // Greyed out
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	}
	ImGui::SameLine();
	ImGui::PushID("EngineErrorFilterBtn");
	if (ImGui::Button("Error")) {
		m_EngineConsoleFilterType ^= 0x10;
	}
	ImGui::PopID();
	ImGui::PopStyleColor(3);

	// Critical filter button
	if (m_EngineConsoleFilterType & 0x20) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.0f, 0.3f, 1.0f)); // Magenta
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.0f, 0.4f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.0f, 0.5f, 1.0f));
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f)); // Greyed out
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	}
	ImGui::SameLine();
	ImGui::PushID("EngineCriticalFilterBtn");
	if (ImGui::Button("Critical")) {
		m_EngineConsoleFilterType ^= 0x20;
	}
	ImGui::PopID();
	ImGui::PopStyleColor(3);

	// Undefined filter button
	if (m_EngineConsoleFilterType & 0x40) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 1.0f)); // Gray
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f)); // Greyed out
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	}
	ImGui::SameLine();
	ImGui::PushID("EngineUndefinedFilterBtn");
	if (ImGui::Button("Undefined")) {
		m_EngineConsoleFilterType ^= 0x40;
	}
	ImGui::PopID();
	ImGui::PopStyleColor(3);

	// Display messages (reading from local storage, no lock needed!)
	if (!m_EngineConsoleLocalMessages.empty())
	{
		ImGui::BeginChild("EngineConsoleScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		// Check if we're at the bottom before rendering new messages
		bool shouldAutoScroll = (ImGui::GetScrollY() >= ImGui::GetScrollMaxY());

		for (const auto& msg : filteredMessages) {
			switch (msg.second.type) {
			case EngineConsole::Message::Type::Trace:
				if (msg.first > 1)
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "[TRACE]: %s [%d]", msg.second.content.c_str(), msg.first);
				else
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "[TRACE]: %s", msg.second.content.c_str());
				break;
			case EngineConsole::Message::Type::Debug:
				if (msg.first > 1)
					ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.8f, 1.0f), "[DEBUG]: %s [%d]", msg.second.content.c_str(), msg.first);
				else
					ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.8f, 1.0f), "[DEBUG]: %s", msg.second.content.c_str());
				break;
			case EngineConsole::Message::Type::Info:
				if (msg.first > 1)
					ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[INFO]: %s [%d]", msg.second.content.c_str(), msg.first);
				else
					ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[INFO]: %s", msg.second.content.c_str());
				break;
			case EngineConsole::Message::Type::Warn:
				if (msg.first > 1)
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[WARN]: %s [%d]", msg.second.content.c_str(), msg.first);
				else
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[WARN]: %s", msg.second.content.c_str());
				break;
			case EngineConsole::Message::Type::Error:
				if (msg.first > 1)
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[ERROR]: %s [%d]", msg.second.content.c_str(), msg.first);
				else
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[ERROR]: %s", msg.second.content.c_str());
				break;
			case EngineConsole::Message::Type::Critical:
				if (msg.first > 1)
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.5f, 1.0f), "[CRITICAL]: %s [%d]", msg.second.content.c_str(), msg.first);
				else
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.5f, 1.0f), "[CRITICAL]: %s", msg.second.content.c_str());
				break;
			case EngineConsole::Message::Type::Undefined:
				if (msg.first > 1)
					ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[UNDEFINED]: %s [%d]", msg.second.content.c_str(), msg.first);
				else
					ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[UNDEFINED]: %s", msg.second.content.c_str());
				break;
			default:
				ImGui::Text("[UNKNOWN]: %s", msg.second.content.c_str());
				break;
			}
		}

		// Auto-scroll to bottom if we were already at the bottom
		if (shouldAutoScroll)
		{
			ImGui::SetScrollHereY(1.0f);  // 1.0f = bottom of the scrollable region
		}

		if (clearConsole)
		{
			m_EngineConsoleLocalMessages.clear();
		}
		ImGui::EndChild();
	}

	ImGui::End();
}

void EditorMain::Render_SceneSettings()
{
	PF_EDITOR_SCOPE("Render_SceneSettings");

	// OPTIMIZATION: Check if window is visible before doing expensive work
	if (!ImGui::Begin("Render Settings", &showSceneSettings)) {
		ImGui::End();
		return;
	}

	// Get active scene
	auto activeSceneOpt = Engine::GetSceneRegistry().GetActiveScene();
	if (!activeSceneOpt.has_value()) {
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No active scene loaded");
		ImGui::End();
		return;
	}

	Scene& activeScene = activeSceneOpt.value().get();
	auto& skyboxSettings = activeScene.GetRenderSettings().skybox;

	// Enable/Disable checkbox
	ImGui::Text("Skybox");
	bool enabled = skyboxSettings.enabled;
	if (ImGui::Checkbox("Enable Skybox", &enabled)) {
		engineService.ExecuteOnEngineThread([enabled]() {
			auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
			scene.GetRenderSettings().skybox.enabled = enabled;
			});
	}

	// Face texture selection (resource dropdowns)
	ImGui::Text("Cubemap Face Textures:");
	ImGui::Spacing();

	const char* faceLabels[6] = {
		"Right (+X)", "Left (-X)", "Top (+Y)",
		"Bottom (-Y)", "Back (+Z)", "Front (-Z)"
	};

	// Helper to check if asset is a texture
	auto is_texture_asset = [](std::string const& assetname) -> bool {
		std::string lower = assetname;
		std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
		static const std::vector<std::string> texture_exts = {
			".png", ".jpg", ".jpeg", ".tga", ".bmp", ".hdr", ".dds"
		};
		for (auto const& ext : texture_exts) {
			if (lower.ends_with(ext)) return true;
		}
		return false;
		};

	bool needsReload = false;

	// Render dropdown for each cubemap face
	for (int i = 0; i < 6; ++i) {
		ImGui::PushID(i);

		// Get current texture name
		std::string current_name = "None";
		if (skyboxSettings.faceTextures[i] != rp::null_guid) {
			rp::BasicIndexedGuid indexed{ skyboxSettings.faceTextures[i], 0 };
			current_name = m_AssetManager->ResolveAssetName(indexed);
			if (current_name.empty()) {
				current_name = skyboxSettings.faceTextures[i].to_hex().substr(0, 8) + "...";
			}
		}

		// Dropdown combo box
		ImGui::SetNextItemWidth(300.0f);
		if (ImGui::BeginCombo(faceLabels[i], current_name.c_str())) {
			// "None" option
			if (ImGui::Selectable("None", skyboxSettings.faceTextures[i] == rp::null_guid)) {
				rp::Guid cleared = rp::null_guid;
				engineService.ExecuteOnEngineThread([i, cleared]() {
					auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
					scene.GetRenderSettings().skybox.faceTextures[i] = cleared;
					scene.GetRenderSettings().skybox.needsReload = true;
					});
				needsReload = true;
			}

			// List all texture assets
			for (auto const& [assetname, indexed_guid] : m_AssetManager->m_AssetNameGuid) {
				// Filter for textures only
				if (!is_texture_asset(assetname)) continue;

				bool selected = (indexed_guid.m_guid == skyboxSettings.faceTextures[i]);
				if (ImGui::Selectable(assetname.c_str(), selected)) {
					// Capture by value for thread safety
					rp::Guid selectedGuid = indexed_guid.m_guid;
					engineService.ExecuteOnEngineThread([i, selectedGuid]() {
						auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
						scene.GetRenderSettings().skybox.faceTextures[i] = selectedGuid;
						scene.GetRenderSettings().skybox.needsReload = true;
						});
					needsReload = true;
				}
				if (selected) ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

		ImGui::PopID();
	}

	// Load button (if textures changed)
	if (ImGui::Button("Load Cubemap")) {
		engineService.ExecuteOnEngineThread([]() {
			auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
			scene.GetRenderSettings().skybox.needsReload = true;
			scene.GetRenderSettings().skybox.enabled = true;
			});
	}

	ImGui::SameLine();

	// Reload button
	if (ImGui::Button("Reload")) {
		engineService.ExecuteOnEngineThread([]() {
			auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
			scene.GetRenderSettings().skybox.needsReload = true;
			});
	}

	// Skybox properties
	float exposure = skyboxSettings.exposure;
	if (ImGui::SliderFloat("Exposure", &exposure, 0.0f, 10.0f)) {
		engineService.ExecuteOnEngineThread([exposure]() {
			auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
			scene.GetRenderSettings().skybox.exposure = exposure;
			});
	}

	glm::vec3 rotation = skyboxSettings.rotation;
	if (ImGui::DragFloat3("Rotation (degrees)", &rotation.x, 1.0f, 0.0f, 360.0f)) {
		engineService.ExecuteOnEngineThread([rotation]() {
			auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
			scene.GetRenderSettings().skybox.rotation = rotation;
			});
	}

	glm::vec3 tint = skyboxSettings.tint;
	if (ImGui::ColorEdit3("Tint", &tint.x)) {
		engineService.ExecuteOnEngineThread([tint]() {
			auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
			scene.GetRenderSettings().skybox.tint = tint;
			});
	}

	ImGui::Text("Status:");
	if (skyboxSettings.cachedCubemapID != 0) {
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Cubemap loaded (GPU ID: %u)", skyboxSettings.cachedCubemapID);
	}
	else {
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No cubemap loaded");
	}

	// Show texture assignments
	int validTextures = 0;
	for (const auto& guid : skyboxSettings.faceTextures) {
		if (guid != rp::null_guid) validTextures++;
	}
	ImGui::Text("Face textures assigned: %d/6", validTextures);

	ImGui::Separator();

	// Lighting settings
	ImGui::Text("Scene Lighting");
	auto& renderSettings = activeScene.GetRenderSettings();

	glm::vec3 ambientLight = renderSettings.ambientLight;
	if (ImGui::ColorEdit3("Ambient Light", &ambientLight.x)) {
		engineService.ExecuteOnEngineThread([ambientLight]() {
			auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
			scene.GetRenderSettings().ambientLight = ambientLight;
			});
	}

	ImGui::Separator();

	// Post-processing settings
	ImGui::Text("Post-Processing");

	float bloomStrength = renderSettings.bloomStrength;
	if (ImGui::SliderFloat("Bloom Strength", &bloomStrength, 0.0f, 0.2f, "%.3f")) {
		engineService.ExecuteOnEngineThread([bloomStrength]() {
			auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
			scene.GetRenderSettings().bloomStrength = bloomStrength;
			});
	}

	const char* toneMapMethods[] = { "None", "Reinhard", "ACES", "Exposure" };
	int toneMapMethod = renderSettings.toneMapMethod;
	if (ImGui::Combo("Tone Mapping", &toneMapMethod, toneMapMethods, 4)) {
		engineService.ExecuteOnEngineThread([toneMapMethod]() {
			auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
			scene.GetRenderSettings().toneMapMethod = toneMapMethod;
			});
	}

	ImGui::Text("HDR Exposure Clamp");
	float exposureMin = renderSettings.exposureMin;
	if (ImGui::SliderFloat("Min Exposure", &exposureMin, 0.0f, 5.0f, "%.2f")) {
		engineService.ExecuteOnEngineThread([exposureMin]() {
			auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
			scene.GetRenderSettings().exposureMin = exposureMin;
			});
	}

	float exposureMax = renderSettings.exposureMax;
	if (ImGui::SliderFloat("Max Exposure", &exposureMax, 0.0f, 5.0f, "%.2f")) {
		engineService.ExecuteOnEngineThread([exposureMax]() {
			auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
			scene.GetRenderSettings().exposureMax = exposureMax;
			});
	}

	ImGui::Separator();

	// Fog settings
	ImGui::Text("Fog");

	auto& fogSettings = renderSettings.fog;
	const char* fogTypes[] = { "None", "Linear", "Exponential", "Exponential Squared" };
	int fogType = static_cast<int>(fogSettings.type);
	if (ImGui::Combo("Fog Mode", &fogType, fogTypes, 4)) {
		engineService.ExecuteOnEngineThread([fogType]() {
			auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
			scene.GetRenderSettings().fog.type = static_cast<FogType>(fogType);
			});
	}

	if (fogSettings.type != FogType::None) {
		glm::vec3 fogColor = fogSettings.color;
		if (ImGui::ColorEdit3("Fog Color", &fogColor.x)) {
			engineService.ExecuteOnEngineThread([fogColor]() {
				auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
				scene.GetRenderSettings().fog.color = fogColor;
				});
		}

		float fogEnd = fogSettings.end;
		if (ImGui::DragFloat("Fog End", &fogEnd, 1.0f, 0.0f, 10000.0f, "%.1f")) {
			engineService.ExecuteOnEngineThread([fogEnd]() {
				auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
				scene.GetRenderSettings().fog.end = fogEnd;
				});
		}

		if (fogSettings.type == FogType::Linear) {
			float fogStart = fogSettings.start;
			if (ImGui::DragFloat("Fog Start", &fogStart, 1.0f, 0.0f, fogSettings.end, "%.1f")) {
				engineService.ExecuteOnEngineThread([fogStart]() {
					auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
					scene.GetRenderSettings().fog.start = fogStart;
					});
			}
		} else {
			float fogDensity = fogSettings.density;
			if (ImGui::DragFloat("Fog Density", &fogDensity, 0.001f, 0.0f, 1.0f, "%.4f")) {
				engineService.ExecuteOnEngineThread([fogDensity]() {
					auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
					scene.GetRenderSettings().fog.density = fogDensity;
					});
			}
		}
	}

	ImGui::Separator();

	ImGui::End();
}

void EditorMain::Render_PhysicsDebugPanel()
{
	PF_EDITOR_SCOPE("Render_PhysicsDebugPanel");

	// OPTIMIZATION: Check if window is visible before doing expensive work
	if (!ImGui::Begin("Physics Debug", &showPhysicsDebug)) {
		ImGui::End();
		return;
	}

	// First-time initialization: sync physics debug rendering state with engine
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		// Execute on engine thread to safely access RenderSystem
		engineService.ExecuteOnEngineThread([enabled = m_PhysicsDebugRenderingEnabled]() {
			Engine::GetRenderSystem().SetJoltDebugRenderingEnabled(enabled);
			Engine::GetRenderSystem().m_SceneRenderer->EnablePhysicsDebugVisualization(enabled);
			spdlog::info("EditorMain: Physics debug rendering initialized (enabled: {})", enabled);
			});
	}

	// Toggle for enabling/disabling Jolt debug rendering (wireframes, velocities, etc.)
	if (ImGui::Checkbox("Enable Debug Rendering", &m_PhysicsDebugRenderingEnabled)) {
		// Execute on engine thread to safely access RenderSystem
		engineService.ExecuteOnEngineThread([enabled = m_PhysicsDebugRenderingEnabled]() {
			Engine::GetRenderSystem().SetJoltDebugRenderingEnabled(enabled);
			Engine::GetRenderSystem().m_SceneRenderer->EnablePhysicsDebugVisualization(enabled);
			});
	}

	// Granular debug visualization controls (PhysicsSystem flags)
	ImGui::Spacing();
	ImGui::Text("Debug Visualization Options:");
	ImGui::Indent();

	static bool drawShapes = true;           // Collision shapes (wireframe)
	static bool drawVelocities = true;       // Velocity vectors
	static bool drawContacts = false;        // Contact points/normals
	static bool drawBoundingBoxes = false;   // AABBs

	if (ImGui::Checkbox("Draw Collision Shapes", &drawShapes)) {
		engineService.ExecuteOnEngineThread([enabled = drawShapes]() {
			PhysicsSystem::Instance().SetDrawShapes(enabled);
			});
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Render collision geometry as wireframes");
	}

	if (ImGui::Checkbox("Draw Velocities", &drawVelocities)) {
		engineService.ExecuteOnEngineThread([enabled = drawVelocities]() {
			PhysicsSystem::Instance().SetDrawVelocities(enabled);
			});
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Show velocity vectors for dynamic bodies");
	}

	if (ImGui::Checkbox("Draw Bounding Boxes", &drawBoundingBoxes)) {
		engineService.ExecuteOnEngineThread([enabled = drawBoundingBoxes]() {
			PhysicsSystem::Instance().SetDrawBoundingBoxes(enabled);
			});
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Show axis-aligned bounding boxes (AABBs)");
	}

	if (ImGui::Checkbox("Draw Contact Points", &drawContacts)) {
		engineService.ExecuteOnEngineThread([enabled = drawContacts]() {
			PhysicsSystem::Instance().SetDrawContacts(enabled);
			});
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Show collision contact points and normals");
	}

	ImGui::Unindent();
	ImGui::Separator();

	if (m_SelectedEntityID == 0) {
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No entity selected");
		ImGui::End();
		return;
	}

	auto& entityHandles = engineService.m_cont->m_entities_snapshot;
	[[maybe_unused]] auto& entityNames = engineService.m_cont->m_names_snapshot;

	for (size_t i = 0; i < entityHandles.size(); ++i) {
		auto ehdl = entityHandles[i];
		uint32_t entityUID = ecs::entity(ehdl).get_uid();
		if (m_SelectedEntityID != entityUID) { continue; }
	}

	// Queue fetch on engine thread
	engineService.ExecuteOnEngineThread([this, entityHandle = engineService.m_cont->m_snapshot_entity_handle]() {
		ecs::entity entity{ entityHandle };
		auto world = Engine::GetWorld();

		// Reset data
		m_PhysicsDebugData = PhysicsDebugData{};

		if (!entity) return;

		// Check for components
		m_PhysicsDebugData.hasRigidBody = world.has_all_components_in_entity<RigidBodyComponent>(entity);
		m_PhysicsDebugData.hasCollider = world.has_any_components_in_entity<BoxCollider, SphereCollider, CapsuleCollider>(entity);

		// Get Jolt body info
		JPH::BodyID bodyID = PhysicsSystem::Instance().GetBodyID(entity);
		m_PhysicsDebugData.hasPhysicsBody = !bodyID.IsInvalid();

		if (m_PhysicsDebugData.hasPhysicsBody) {
			auto& bodyInterface = PhysicsSystem::Instance().GetBodyInterface();
			m_PhysicsDebugData.bodyID = bodyID.GetIndexAndSequenceNumber();

			// Body state
			m_PhysicsDebugData.isBodyActive = bodyInterface.IsActive(bodyID);
			m_PhysicsDebugData.isSleeping = !m_PhysicsDebugData.isBodyActive;

			// Position and rotation from Jolt
			JPH::Vec3 joltPos = bodyInterface.GetPosition(bodyID);
			JPH::Quat joltRot = bodyInterface.GetRotation(bodyID);
			m_PhysicsDebugData.joltPosition = PhysicsUtils::ToGLM(joltPos);
			m_PhysicsDebugData.joltRotation = PhysicsUtils::JoltQuatToEulerDegrees(joltRot);

			// Velocities
			m_PhysicsDebugData.linearVelocity = PhysicsUtils::ToGLM(bodyInterface.GetLinearVelocity(bodyID));
			m_PhysicsDebugData.angularVelocity = PhysicsUtils::ToGLM(bodyInterface.GetAngularVelocity(bodyID));

			// Motion type
			JPH::EMotionType motionType = bodyInterface.GetMotionType(bodyID);
			switch (motionType) {
			case JPH::EMotionType::Static: m_PhysicsDebugData.motionType = "Static"; break;
			case JPH::EMotionType::Kinematic: m_PhysicsDebugData.motionType = "Kinematic"; break;
			case JPH::EMotionType::Dynamic: m_PhysicsDebugData.motionType = "Dynamic"; break;
			default: m_PhysicsDebugData.motionType = "Unknown"; break;
			}

			// Body properties
			m_PhysicsDebugData.friction = bodyInterface.GetFriction(bodyID);
			m_PhysicsDebugData.restitution = bodyInterface.GetRestitution(bodyID);
			m_PhysicsDebugData.joltGravFactor = bodyInterface.GetGravityFactor(bodyID);

		}

		// RigidBody component data
		if (m_PhysicsDebugData.hasRigidBody) {
			auto& rb = entity.get<RigidBodyComponent>();
			m_PhysicsDebugData.mass = rb.mass;
			m_PhysicsDebugData.linearDamping = rb.linearDamping;
			m_PhysicsDebugData.angularDamping = rb.angularDrag;
			m_PhysicsDebugData.gravityFactor = rb.gravityFactor;
			m_PhysicsDebugData.useGravity = rb.useGravity;
		}

		// Collider data
		if (world.has_all_components_in_entity<BoxCollider>(entity)) {
			auto& collider = entity.get<BoxCollider>();
			m_PhysicsDebugData.colliderType = "Box";
			m_PhysicsDebugData.isTrigger = collider.isTrigger;
			m_PhysicsDebugData.colliderSize = collider.size;
			m_PhysicsDebugData.friction = collider.friction;
			m_PhysicsDebugData.restitution = collider.restitution;
		}
		else if (world.has_all_components_in_entity<SphereCollider>(entity)) {
			auto& collider = entity.get<SphereCollider>();
			m_PhysicsDebugData.colliderType = "Sphere";
			m_PhysicsDebugData.isTrigger = collider.isTrigger;
			m_PhysicsDebugData.colliderRadius = collider.radius;
			m_PhysicsDebugData.friction = collider.friction;
			m_PhysicsDebugData.restitution = collider.restitution;
		}
		else if (world.has_all_components_in_entity<CapsuleCollider>(entity)) {
			auto& collider = entity.get<CapsuleCollider>();
			m_PhysicsDebugData.colliderType = "Capsule";
			m_PhysicsDebugData.isTrigger = collider.isTrigger;
			m_PhysicsDebugData.colliderRadius = collider.GetRadius();
			m_PhysicsDebugData.colliderHeight = collider.GetHeight();
			m_PhysicsDebugData.friction = collider.friction;
			m_PhysicsDebugData.restitution = collider.restitution;
		}
		});

	// Display physics information
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));

	// Component Status
	ImGui::SeparatorText("Component Status");
	ImGui::Text("Has RigidBody:    %s", m_PhysicsDebugData.hasRigidBody ? "Yes" : "No");
	ImGui::Text("Has Collider:     %s", m_PhysicsDebugData.hasCollider ? "Yes" : "No");
	ImGui::Text("Has Physics Body: %s", m_PhysicsDebugData.hasPhysicsBody ? "Yes" : "No");

	if (!m_PhysicsDebugData.hasPhysicsBody) {
		ImGui::Separator();
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "No physics body created for this entity");
		ImGui::TextWrapped("Add a collider component to create a physics body.");
		ImGui::PopStyleColor();
		ImGui::End();
		return;
	}

	// Jolt Body Info
	ImGui::SeparatorText("Jolt Body Info");
	ImGui::Text("Body ID:          %u", m_PhysicsDebugData.bodyID);
	ImGui::Text("Motion Type:      %s", m_PhysicsDebugData.motionType.c_str());
	ImGui::Text("Collider Type:    %s", m_PhysicsDebugData.colliderType.c_str());
	ImGui::Text("Is Trigger:       %s", m_PhysicsDebugData.isTrigger ? "Yes" : "No");

	// Body State
	ImGui::SeparatorText("Body State");
	if (m_PhysicsDebugData.isBodyActive) {
		ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Active");
	}
	else {
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Sleeping");
	}

	// Position & Rotation (from Jolt)
	ImGui::SeparatorText("Transform (Jolt)");
	ImGui::Text("Position:  (%.2f, %.2f, %.2f)",
		m_PhysicsDebugData.joltPosition.x,
		m_PhysicsDebugData.joltPosition.y,
		m_PhysicsDebugData.joltPosition.z);
	ImGui::Text("Rotation:  (%.2f, %.2f, %.2f)",
		m_PhysicsDebugData.joltRotation.x,
		m_PhysicsDebugData.joltRotation.y,
		m_PhysicsDebugData.joltRotation.z);

	// Velocities
	ImGui::SeparatorText("Velocities");
	ImGui::Text("Linear:    (%.2f, %.2f, %.2f)",
		m_PhysicsDebugData.linearVelocity.x,
		m_PhysicsDebugData.linearVelocity.y,
		m_PhysicsDebugData.linearVelocity.z);
	ImGui::Text("Angular:   (%.2f, %.2f, %.2f)",
		m_PhysicsDebugData.angularVelocity.x,
		m_PhysicsDebugData.angularVelocity.y,
		m_PhysicsDebugData.angularVelocity.z);
	ImGui::Text("Jolt Grav: %.2f m/s^2", m_PhysicsDebugData.joltGravFactor);

	float linearSpeed = glm::length(m_PhysicsDebugData.linearVelocity);
	float angularSpeed = glm::length(m_PhysicsDebugData.angularVelocity);
	ImGui::Text("Linear Speed:  %.2f m/s", linearSpeed);
	ImGui::Text("Angular Speed: %.2f rad/s", angularSpeed);

	// RigidBody Properties
	if (m_PhysicsDebugData.hasRigidBody) {
		ImGui::SeparatorText("RigidBody Properties");
		ImGui::Text("Mass:             %.2f kg", m_PhysicsDebugData.mass);
		ImGui::Text("Linear Damping:   %.3f", m_PhysicsDebugData.linearDamping);
		ImGui::Text("Angular Damping:  %.3f", m_PhysicsDebugData.angularDamping);
		ImGui::Text("Gravity Factor:   %.2f", m_PhysicsDebugData.gravityFactor);
		ImGui::Text("Use Gravity:      %s", m_PhysicsDebugData.useGravity ? "Yes" : "No");
	}

	// Collider Properties
	ImGui::SeparatorText("Collider Properties");
	ImGui::Text("Type:             %s", m_PhysicsDebugData.colliderType.c_str());
	ImGui::Text("Friction:         %.3f", m_PhysicsDebugData.friction);
	ImGui::Text("Restitution:      %.3f", m_PhysicsDebugData.restitution);

	if (m_PhysicsDebugData.colliderType == "Box") {
		ImGui::Text("Size:             (%.2f, %.2f, %.2f)",
			m_PhysicsDebugData.colliderSize.x,
			m_PhysicsDebugData.colliderSize.y,
			m_PhysicsDebugData.colliderSize.z);
	}
	else if (m_PhysicsDebugData.colliderType == "Sphere") {
		ImGui::Text("Radius:           %.2f", m_PhysicsDebugData.colliderRadius);
	}
	else if (m_PhysicsDebugData.colliderType == "Capsule") {
		ImGui::Text("Radius:           %.2f", m_PhysicsDebugData.colliderRadius);
		ImGui::Text("Height:           %.2f", m_PhysicsDebugData.colliderHeight);
	}

	ImGui::PopStyleColor();
	ImGui::End();
}

void EditorMain::Render_ExporterSettings() 
{
	if (showExporter) {
		showExporter = false;
		ImGui::OpenPopup("Build Menu");
	}
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	static bool needs_repositioning{};
	if (needs_repositioning) {
		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		needs_repositioning = false;
	}
	else {
		//ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	}
	// Begin the popup modal
	if (ImGui::BeginPopupModal("Build Menu", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		//temp static
		static std::atomic_bool buildStarted{};
		static BuildConfiguration config{ [](std::string name, std::string dir) {
			auto config1 = BuildManager::LoadBuildConfiguration();
			if (config1.output_name.empty()) {
				config1.output_name = name;
			};
			if (config1.output_dir.empty() || !std::filesystem::exists(config1.output_dir)) {
				config1.output_dir = dir;
			};
			std::string iconfpath = std::string(Engine::getWorkingDir().data()) + "/" + config1.icon_relative_path;
			if (config1.icon_relative_path.empty() || !std::filesystem::exists(iconfpath)) {
				config1.icon_relative_path = "/Icon.ico";
			};
			return config1;
			}(Editor::GetInstance().GetConfig().workspace_name, std::string(Engine::getWorkingDir().data()) + "/build")};
		static std::shared_ptr<BuildContext> buildCtx{ std::make_shared<BuildContext>() };
		static std::unique_ptr<std::future<void>> fut_ptr{}; //hack

		if (!buildStarted) {
			ImGui::Text("Build Settings");
			ImGui::Separator();

			ImguiInspectTypeRenderer::present(config, "Build Configuration                                              ");//i dunno how to resize this
			ImGui::Separator();
			if (ImGui::Button("Build", ImVec2(120, 0)))
			{
				buildStarted.store(true);
				BuildManager::SaveBuildConfiguration(config);
				needs_repositioning = true;
				fut_ptr.reset(new std::future<void> (BuildManager::BuildAsync(config, buildCtx))); //hack
			}
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Save", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
				BuildManager::SaveBuildConfiguration(config);
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
			}
		}
		else {
			needs_repositioning = true;
			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(0, 122, 204, 255)); // blue fill 
			ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(40, 40, 40, 255)); // dark background
			ImGui::Text("Build Progress");
			float progress = buildCtx->m_progress100 / 100.f;
			ImGui::Dummy(ImVec2(10, 5));
			ImGui::SameLine();
			ImGui::ProgressBar(progress, ImVec2(750, 25));
			ImGui::SameLine();
			ImGui::Dummy(ImVec2(10, 5));
			ImGui::PopStyleColor(2);
			ImGui::Separator();
			if (fut_ptr->valid()&&fut_ptr->wait_for(std::chrono::nanoseconds(0))==std::future_status::ready) {
				ImGui::Text(buildCtx->m_state == BuildState::FAILED ? "Build Failed!" : "Build Success!");
				if (ImGui::Button("OK", ImVec2(120, 0))) {
					fut_ptr->get();
					ImGui::CloseCurrentPopup();
					buildStarted = false;
				}
			}
			else if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				buildCtx->m_state = BuildState::ABORTED;
				ImGui::CloseCurrentPopup();
				buildStarted = false;
				fut_ptr->wait();
			}
		}
		ImGui::EndPopup();
	}
}

void EditorMain::Render_ImporterSettings()
{
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	// Begin the popup modal
	if (ImGui::BeginPopupModal("DescriptorInspector", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Importer Settings");
		ImGui::Separator();

		auto& desc{ m_AssetManager->GetImportSettings() };
		rp::ResourceTypeImporterRegistry::Serialize(desc.m_desc_importer_hash, "imgui", m_AssetManager->GetImportSettingsPath(), desc);

		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			m_AssetManager->UnloadImportSetting();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			m_AssetManager->ClearImportSetting();
		}

		ImGui::EndPopup();
	}
}

void EditorMain::Render_AboutUI()
{
	// Render About Modal
	if (showAboutModal)
	{
		ImGui::OpenPopup("About");
		showAboutModal = false;
	}
	if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Basil Editor");
		ImGui::Separator();
		ImGui::Text("Version 1.0.0");
		ImGui::Text("Developed by team PASTA");
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}

}
