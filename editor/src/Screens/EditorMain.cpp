/******************************************************************************/
/*!
\file   EditorMain.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)
		Eirwen (c.lau\@digipen.edu)
		Hai Jie (haijie.w\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contain the implmentation for the EditorMain class, which is the
main editor screen handling the viewport, entity management, and various editor panels.
It integrates with the rendering system and ECS to provide a functional editor environment.
It includes features like scene exploration, entity inspection, and camera controls. It allows
for creating, selecting, and manipulating entities within the scene.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#include <Render/Render.h>
#include <Render/Camera.h>
#include <Engine.hpp>
#include <Component/Transform.hpp>
#include <Component/MaterialOverridesComponent.hpp>
#include <Component/PrefabComponent.hpp>
#include <System/PrefabSystem.hpp>
#include <System/Audio.hpp>
#include <Resources/PrimitiveGenerator.h>
#include <Resources/Material.h>
#include <Resources/Texture.h>
#include <Input/InputManager.h>
#include <Pipeline/DebugRenderPass.h>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <chrono>
#include <regex>
#include <algorithm>
#include <functional>

#include "Screens/EditorMain.hpp"
#include <filesystem>

#include "Editor.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "Core/Window.h"
#include "GLFW/glfw3.h"
#include "Profiler/profiler.hpp"
#include "Bindings/MANAGED_CONSOLE.hpp"
#include "rsc-core/rp.hpp"
#include <descriptors/audio.hpp>
#include <serialization/serializer.h>

#include <glm/gtc/type_ptr.hpp>
#include "Physics/Physics_System.h"
#include "Manager/ObjectManager.hpp"
#include <components/behaviour.hpp>
#include <System/BehaviourSystem.hpp>
#include <Manager/MonoEntityManager.hpp>
#include <Manager/MonoReflectionRegistry.hpp>
#include <Manager/MonoTypeRegistry.hpp>
#include <Messaging/Messaging_System.h>
#include "MonoResolver/MonoTypeDescriptor.hpp"
#include "Manager/MonoImGuiRenderer.hpp"
#include <Scene/Scene.hpp>

#include "Particles/ParticleComponent.h"

//#include <Component/RelationshipComponent.hpp>

#define UNREF_PARAM(x) x;

RegisterImguiDescriptorInspector(ModelDescriptor);
RegisterImguiDescriptorInspector(TextureDescriptor);

#include <ImGuizmo.h>

//PhysicsSystem PhysSys;
JPH::Body* floorplan; // Delete this after m1
JPH::BodyID sphere_id;

// Shader uniform parsing helpers
namespace {
	struct ShaderUniforms {
		std::set<std::string> samplers;
		std::set<std::string> floats;
		std::set<std::string> vec3s;
		std::set<std::string> vec4s;
	};

	ShaderUniforms ParseShaderUniforms(std::string const& shader_path) {
		ShaderUniforms uniforms;

		std::ifstream file(shader_path);
		if (!file.is_open()) return uniforms;

		std::string line;
		std::regex sampler_regex(R"(uniform\s+sampler2D\s+(\w+)\s*;)");
		std::regex float_regex(R"(uniform\s+float\s+(\w+)\s*;)");
		std::regex vec3_regex(R"(uniform\s+vec3\s+(\w+)\s*;)");
		std::regex vec4_regex(R"(uniform\s+vec4\s+(\w+)\s*;)");
		std::smatch match;

		while (std::getline(file, line)) {
			if (std::regex_search(line, match, sampler_regex)) {
				uniforms.samplers.insert(match[1].str());
			}
			if (std::regex_search(line, match, float_regex)) {
				uniforms.floats.insert(match[1].str());
			}
			if (std::regex_search(line, match, vec3_regex)) {
				uniforms.vec3s.insert(match[1].str());
			}
			if (std::regex_search(line, match, vec4_regex)) {
				uniforms.vec4s.insert(match[1].str());
			}
		}

		return uniforms;
	}

	void SyncMaterialProperties(MaterialDescriptor& desc, std::string const& shader_dir) {
		ShaderUniforms all_uniforms;

		// Parse fragment shaders
		std::string frag_path = shader_dir + "/" + desc.material.frag_name;

		ShaderUniforms frag_uniforms = ParseShaderUniforms(frag_path);

		// Merge uniforms from both shaders
		all_uniforms.samplers.insert(frag_uniforms.samplers.begin(), frag_uniforms.samplers.end());
		all_uniforms.floats.insert(frag_uniforms.floats.begin(), frag_uniforms.floats.end());
		all_uniforms.vec3s.insert(frag_uniforms.vec3s.begin(), frag_uniforms.vec3s.end());
		all_uniforms.vec4s.insert(frag_uniforms.vec4s.begin(), frag_uniforms.vec4s.end());

		// Exclude engine-provided uniforms (set by renderer at runtime)
		static const std::set<std::string> excluded_uniforms = {
			// Camera/View
			"u_ViewPos",
			"u_ViewMatrix",
			"u_ProjectionMatrix",
			"u_ViewProjection",
			"u_CameraPos",

			// Transform
			"u_ModelMatrix",
			"u_Model",
			"u_Transform",
			"u_NormalMatrix",

			// Time
			"u_Time",
			"u_DeltaTime",

			// Lighting (system-wide)
			"u_LightCount",
			"u_LightPositions",
			"u_LightColors"
		};

		// Remove excluded uniforms from all property sets
		for (auto const& name : excluded_uniforms) {
			all_uniforms.samplers.erase(name);
			all_uniforms.floats.erase(name);
			all_uniforms.vec3s.erase(name);
			all_uniforms.vec4s.erase(name);
		}

		// Sync texture_properties
		{
			std::unordered_map<std::string, rp::Guid> new_map;
			for (auto const& name : all_uniforms.samplers) {
				if (desc.material.texture_properties.count(name)) {
					new_map[name] = desc.material.texture_properties[name]; // Keep existing
				} else {
					new_map[name] = rp::Guid{}; // Add new with empty GUID
				}
			}
			desc.material.texture_properties = std::move(new_map);
		}

		// Sync float_properties
		{
			std::unordered_map<std::string, float> new_map;
			for (auto const& name : all_uniforms.floats) {
				if (desc.material.float_properties.count(name)) {
					new_map[name] = desc.material.float_properties[name];
				} else {
					new_map[name] = 0.0f;
				}
			}
			desc.material.float_properties = std::move(new_map);
		}

		// Sync vec3_properties
		{
			std::unordered_map<std::string, glm::vec3> new_map;
			for (auto const& name : all_uniforms.vec3s) {
				if (desc.material.vec3_properties.count(name)) {
					new_map[name] = desc.material.vec3_properties[name];
				} else {
					new_map[name] = glm::vec3(0.0f);
				}
			}
			desc.material.vec3_properties = std::move(new_map);
		}

		// Sync vec4_properties
		{
			std::unordered_map<std::string, glm::vec4> new_map;
			for (auto const& name : all_uniforms.vec4s) {
				if (desc.material.vec4_properties.count(name)) {
					new_map[name] = desc.material.vec4_properties[name];
				} else {
					new_map[name] = glm::vec4(0.0f);
				}
			}
			desc.material.vec4_properties = std::move(new_map);
		}
	}
}

EditorMain::EditorMain(GLFWwindow* _window) : Screen(_window)
{

}

void EditorMain::init()
{
	// Set window title
	glfwSetWindowTitle(window, (Editor::GetInstance().GetConfig().workspace_name + " | No Scene").c_str());
	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* v_mode = glfwGetVideoMode(primaryMonitor);
	UNREF_PARAM(primaryMonitor);
	UNREF_PARAM(v_mode);
	// Set maximized
	glfwMaximizeWindow(window);

	m_AssetManager = std::make_unique<AssetManager>(Editor::GetInstance().GetConfig().project_workingDir + "/assets", Editor::GetInstance().GetConfig().project_workingDir + "/.imports");
	

	// Register custom audio inspector with import settings
	{
		constexpr auto audio_type_hash = rp::utility::type_hash<AudioDescriptor>::value();
		rp::ResourceTypeImporterRegistry::RegisterSerializer(audio_type_hash, "imgui",
			[this](std::string const& str, std::byte* data) {
				AudioDescriptor& desc = *reinterpret_cast<AudioDescriptor*>(data);

				// Render descriptor_base
				ImGui::SeparatorText("Base Properties");
				ImGui::Text("GUID: %s", desc.base.m_guid.to_hex().c_str());
				ImGui::Text("Name: %s", desc.base.m_name.c_str());
				ImGui::Text("Source: %s", desc.base.m_source.c_str());

				// Audio properties
				ImGui::SeparatorText("Audio Import Settings");

				ImGui::Checkbox("3D Sound", &desc.audio.is3D);
				ImGui::SameLine();
				ImGui::TextDisabled("(?)");
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Enable spatial 3D audio positioning");
				}

				ImGui::Checkbox("Streaming", &desc.audio.isStreaming);
				ImGui::SameLine();
				ImGui::TextDisabled("(?)");
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Stream from disk (recommended for music/long files)");
				}

				ImGui::Checkbox("Loop", &desc.audio.isLooping);
				ImGui::SameLine();
				ImGui::TextDisabled("(?)");
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Loop playback continuously");
				}

				// Metadata (read-only)
				ImGui::SeparatorText("Audio Metadata");
				if (desc.audio.duration > 0.0f) {
					ImGui::Text("Duration: %.2f seconds", desc.audio.duration);
				}
				if (desc.audio.sampleRate > 0) {
					ImGui::Text("Sample Rate: %d Hz", desc.audio.sampleRate);
				}
				if (desc.audio.channels > 0) {
					ImGui::Text("Channels: %d", desc.audio.channels);
				}
			});
	}

	// Register custom material inspector with texture dropdowns (captures 'this')
	{
		constexpr auto mat_type_hash = rp::utility::type_hash<MaterialDescriptor>::value();
		rp::ResourceTypeImporterRegistry::RegisterSerializer(mat_type_hash, "imgui",
			[this](std::string const& str, std::byte* data) {
				MaterialDescriptor& desc = *reinterpret_cast<MaterialDescriptor*>(data);
				AssetManager* assets = this->GetAssetManager();

				// Cache to track last synced shader names per material
				struct ShaderCache {
					std::string vert_name;
					std::string frag_name;
				};
				static std::unordered_map<std::string, ShaderCache> shader_cache;

				// Check if shader names changed
				std::string material_id = desc.base.m_guid.to_hex();
				bool needs_sync = false;

				auto it = shader_cache.find(material_id);
				if (it == shader_cache.end()) {
					// First time seeing this material
					needs_sync = true;
				} else {
					// Check if shader names changed
					if (it->second.vert_name != desc.material.vert_name ||
						it->second.frag_name != desc.material.frag_name) {
						needs_sync = true;
					}
				}

				// Only sync if needed
				if (needs_sync) {
					std::string shader_dir = Editor::GetInstance().GetConfig().project_workingDir + "/assets/shaders";
					SyncMaterialProperties(desc, shader_dir);

					// Update cache
					shader_cache[material_id] = {desc.material.vert_name, desc.material.frag_name};
				}

				// Render descriptor_base
				ImGui::SeparatorText("Base Properties");
				ImGui::Text("GUID: %s", desc.base.m_guid.to_hex().c_str());
				ImGui::Text("Name: %s", desc.base.m_name.c_str());
				ImGui::Text("Importer: %s", desc.base.m_importer.c_str());

				// Material properties
				ImGui::SeparatorText("Material Properties");

				char vert_buf[256];
				strncpy(vert_buf, desc.material.vert_name.c_str(), sizeof(vert_buf));
				if (ImGui::InputText("Vertex Shader", vert_buf, sizeof(vert_buf))) {
					desc.material.vert_name = vert_buf;
				}

				char frag_buf[256];
				strncpy(frag_buf, desc.material.frag_name.c_str(), sizeof(frag_buf));
				if (ImGui::InputText("Fragment Shader", frag_buf, sizeof(frag_buf))) {
					desc.material.frag_name = frag_buf;
				}

				ImGui::ColorEdit3("Albedo", &desc.material.albedo.x);
				ImGui::SliderFloat("Metallic", &desc.material.metallic, 0.0f, 1.0f);
				ImGui::SliderFloat("Roughness", &desc.material.roughness, 0.0f, 1.0f);

				// Blend mode selector
				const char* blend_modes[] = { "Opaque", "Transparent" };
				const char* current_blend_mode = (desc.material.blend_mode == 0) ? "Opaque" : "Transparent";
				if (ImGui::BeginCombo("Blend Mode", current_blend_mode)) {
					for (int i = 0; i < 2; i++) {
						bool is_selected = (desc.material.blend_mode == i);
						if (ImGui::Selectable(blend_modes[i], is_selected)) {
							desc.material.blend_mode = i;
						}
						if (is_selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				// Texture properties with dropdowns
				ImGui::SeparatorText("Texture Properties");

				// Helper to check if asset is a texture (by base name extension)
				auto is_texture_asset = [](std::string const& assetname) -> bool {
					std::string lower = assetname;
					std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

					static const std::vector<std::string> texture_exts = {
						".png", ".jpg", ".jpeg",
						".tga", ".bmp", ".hdr", ".dds"
					};

					for (auto const& ext : texture_exts) {
						if (lower.ends_with(ext)) return true;
					}
					return false;
				};

				// Render each texture property
				std::vector<std::string> keys_to_remove;
				for (auto& [key, guid] : desc.material.texture_properties) {
					ImGui::PushID(key.c_str());

					std::string current_name = "None";
					if (guid != rp::Guid{}) {
						rp::BasicIndexedGuid indexed{guid, 0};
						current_name = assets->ResolveAssetName(indexed);
						if (current_name.empty()) current_name = guid.to_hex().substr(0, 8) + "...";
					}

					if (ImGui::BeginCombo(key.c_str(), current_name.c_str())) {
						if (ImGui::Selectable("None", guid == rp::Guid{})) {
							guid = rp::Guid{};
						}

						// Iterate through all imported assets (matches Resources Browser behavior)
						for (auto const& [assetname, indexed_guid] : assets->m_AssetNameGuid) {
							// Only show texture assets
							if (!is_texture_asset(assetname)) continue;

							// Use the asset name directly (it's already the base name)
							bool selected = (indexed_guid.m_guid == guid);
							if (ImGui::Selectable(assetname.c_str(), selected)) {
								guid = indexed_guid.m_guid;
							}
							if (selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}

					// Remove button
					ImGui::SameLine();
					if (ImGui::SmallButton("X")) {
						keys_to_remove.push_back(key);
					}
					
					ImGui::PopID();
				}

				// Remove marked keys
				for (auto const& key : keys_to_remove) {
					desc.material.texture_properties.erase(key);
				}

				// Add new texture property
				static char new_texture_key[256] = "";
				ImGui::InputText("New Texture Property", new_texture_key, sizeof(new_texture_key));
				ImGui::SameLine();
				if (ImGui::Button("Add") && strlen(new_texture_key) > 0) {
					desc.material.texture_properties[new_texture_key] = rp::Guid{};
					new_texture_key[0] = '\0';
				}

				// Float properties
				if (!desc.material.float_properties.empty()) {
					ImGui::SeparatorText("Float Properties");
					for (auto& [key, value] : desc.material.float_properties) {
						ImGui::SliderFloat(key.c_str(), &value, 0.0f, 10.0f);
					}
				}

				// Vec3 properties
				if (!desc.material.vec3_properties.empty()) {
					ImGui::SeparatorText("Vec3 Properties");
					for (auto& [key, value] : desc.material.vec3_properties) {
						ImGui::ColorEdit3(key.c_str(), &value.x);
					}
				}

				// Vec4 properties
				if (!desc.material.vec4_properties.empty()) {
					ImGui::SeparatorText("Vec4 Properties");
					for (auto& [key, value] : desc.material.vec4_properties) {
						ImGui::ColorEdit4(key.c_str(), &value.x);
					}
				}
			});
	}

	// Set decoration on
	glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);

	// Initialize Editor Camera
	m_EditorCamera = std::make_shared<EditorCamera>();
	m_EditorCamera->SetPosition(glm::vec3(0.0f, 5.0f, 10.0f));
	// Set rotation to look towards origin from (0,5,10): pitch down, yaw to face -Z direction
	m_EditorCamera->SetRotation(glm::vec3(-25.0f, -90.0f, 0.0f));
	// Set orbit target to origin so orbit mode works
	m_EditorCamera->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));

	CameraSystem::SetAuxCamera(m_EditorCamera);
	CameraSystem::SetActiveCamera(CameraSystem::CameraType::AUX);

	glfwMakeContextCurrent(nullptr);
	// init engine
	engineService.init();
	while (Engine::GetState() == Engine::Info::State::Init);
	engineService.block();
	glfwMakeContextCurrent(Editor::GetInstance().GetWindowPtr());
	engineService.ExecuteOnEngineThread([&]() {
		messagingSystem.Subscribe(CAMERA_CALCULATION_UPDATE, [&](std::unique_ptr<Message> GuizmoMessage) {
			GuizmoViewMec4 = static_cast<Camera_Calculation_Update*>(GuizmoMessage.get())->viewMat4;
			GuizmoprojectionMat4 = static_cast<Camera_Calculation_Update*>(GuizmoMessage.get())->projectionMat4;
			}, nullptr);
		PhysicsSystem::Instance().isActive = false;
		spdlog::info("Physics Active");
		});
	CreateDemoScene();

	SetupUnityStyle();
	//std::jthread jth(&Engine::Update);
	// Note: ImGui callbacks are already set up in main.cpp
	// We need to chain our input handling with ImGui's callbacks
	// InputManager will be updated manually in render() to avoid conflicts

	// Load asset browser icon textures
	LoadAssetIcons();

	MonoEntityManager::GetInstance().Attach();

	engineService.set_on_load();
	engineService.set_on_unload();
}


void EditorMain::update()
{
	if (!active) return;
	std::lock_guard lg{ engineService.m_cont->m_mtx }; //wait for snapshot
}

void EditorMain::render()
{
	if (!active) return;

	ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_PassthruCentralNode;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoDecoration;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("DockSpaceHost", nullptr, host_flags);

	Render_MenuBar(); // The menu bar is attached to the dockspace therefore is fixed and should not be outside 
	Render_StartStop(); // The startstop bar is attached to the dockspace therefore is fixed and should not be outside where it will become dockable

	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

	// kill bluish empty dockspace background
	ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, IM_COL32(0, 0, 0, 0));
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dock_flags);
	ImGui::PopStyleColor();

	ImGui::End();

	ImGui::PopStyleVar(3);


	static bool firstRun = true;
	if (firstRun && ImGui::DockBuilderGetNode(dockspace_id))
	{
		Setup_Dockspace(dockspace_id);
	}

	ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 4.0f);
	ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.157f, 0.157f, 0.157f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.235f, 0.235f, 0.235f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.337f, 0.612f, 0.839f, 1.0f));

	if (showSceneExplorer)
		Render_SceneExplorer();
	if (showConsole)
		Render_Console();
	if (showProfiler)
		Render_Profiler();
	if (showInspector)
		Render_Inspector();
	if (showSkyboxSettings)
		Render_SkyboxSettings();
	Render_PhysicsDebugPanel();
	Render_Scene();
	Render_Game();
	Render_CameraControls();
	Render_AssetBrowser();

	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar();

	engineService.m_cont->m_container_is_presentable.release();
	engineService.start();
}



void EditorMain::cleanup()
{
	//PhysSys.GetBodyInterface().RemoveBody(sphere_id);

	//// Destroy the sphere. After this the sphere ID is no longer valid.
	//PhysSys.GetBodyInterface().DestroyBody(sphere_id);

	// Remove and destroy the floor
	//PhysSys.GetBodyInterface().RemoveBody(floorplan->GetID());
	//PhysSys.GetBodyInterface().DestroyBody(floorplan->GetID());


	engineService.ExecuteOnEngineThread([]() {
		PhysicsSystem::Instance().Exit();
		spdlog::info("Physics Exit");
		});
	//m_AssetManager.reset(nullptr);
	//Engine::Exit();
	///Engine::
	engineService.release();
}

void EditorMain::Show()
{
	active = true;
}

void EditorMain::Close()
{
	active = false;
}

bool EditorMain::isWindowClosed()
{
	return glfwWindowShouldClose(window);
}

bool EditorMain::Activate()
{
	return true;
}


void EditorMain::Render_Inspector()
{
	ImGui::Begin("Inspector", nullptr);

	// Show selected entity information
	if (m_SelectedEntityID == 0u) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.5f - 20);
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("No object selected").x) * 0.5f);
		ImGui::Text("No object selected");
		ImGui::PopStyleColor();
		ImGui::End();
		return;
	}


	ImGui::Text("Selected Entity");
	ImGui::Separator();
	ImGui::Text("Object ID: %u", m_SelectedEntityID);

	auto& entities = engineService.m_cont->m_entities_snapshot;
	auto& entityNames = engineService.m_cont->m_names_snapshot;

	if (auto it{ std::find_if(entities.begin(), entities.end(), [this](std::size_t ehdl) {return ecs::entity(ehdl).get_uid() == m_SelectedEntityID; }) }; it != entities.end()) {
		// Show entity components
		ImGui::Text("Entity UID: %llu", m_SelectedEntityID);
		ImGui::Text("Entity Scene ID: %llu", m_SelectedNodeID);

		auto i = it - entities.begin();

		// Object name header (like Unity)
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 6));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.157f, 0.157f, 0.157f, 1.0f));
		auto const& currentName = entityNames[i];

		char nameBuffer[256];
		snprintf(nameBuffer, sizeof(nameBuffer), currentName.c_str(), currentName.size());

		ImGui::PushItemWidth(-1); // Full width

		// Input text with callback flags
		ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue |
			ImGuiInputTextFlags_AutoSelectAll;

		if (ImGui::InputText("##EntityName", nameBuffer, 256, flags))
		{
			// User pressed Enter
			std::string newName(nameBuffer);
			engineService.ExecuteOnEngineThread([newName, this]() {
				ecs::world world = Engine::GetWorld();
				for (auto& entity : world.get_all_entities()) {
					if (entity.get_uid() == m_SelectedEntityID) {
						entity.name() = newName;
						break;
					}
				}
				spdlog::info("Renamed entity to: {}", newName);
				});
		}

		// Also handle when field loses focus
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			std::string newName(nameBuffer);
			if (newName != currentName)
			{
				engineService.ExecuteOnEngineThread([newName, this]() {
					ecs::world world = Engine::GetWorld();
					for (auto& entity : world.get_all_entities()) {
						if (entity.get_uid() == m_SelectedEntityID) {
							entity.name() = newName;
							break;
						}
					}
					spdlog::info("Renamed entity to: {}", newName);
					});
			}
		}

		ImGui::PopItemWidth();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// renders all reflectible components
		Render_Components();

		ImGui::Separator();

		// Action buttons
		if (ImGui::Button("Clear Selection")) {
			ClearEntitySelection();
		}
		if (ImGui::Button("Add Component")) {
			ImGui::OpenPopup("Add Component Popup");
		}
		if (ImGui::BeginPopup("Add Component Popup")) {
			Render_Add_Component_Menu();
			ImGui::EndPopup();
		}
	}
	else {
		ImGui::Text("Selected entity not found in world!");
		ImGui::Text("(Entity may have been deleted)");
		if (ImGui::Button("Clear Selection")) {
			ClearEntitySelection();
		}
	}

	ImGui::End();
}

void EditorMain::Render_Components()
{
	std::unique_lock ul{ engineService.m_cont->m_mtx };
	auto& component_list{ engineService.m_cont->m_component_list_snapshot };
	auto& type_map{ ReflectionRegistry::types() };
	auto& internal_type_map{ ReflectionRegistry::InternalID() };

	// Check if selected entity is a prefab instance
	m_PrefabContext = PrefabOverrideContext{};  // Reset context
	m_LoadedPrefabData.reset();  // Clear previous prefab data

	// Find PrefabComponent if it exists
	ReflectionRegistry::TypeID prefab_component_id{};
	auto prefabIt = internal_type_map.find(entt::type_index<PrefabComponent>::value());
	if (prefabIt != internal_type_map.end())
	{
		prefab_component_id = prefabIt->second;
		auto prefabCompIt = std::find_if(component_list.begin(), component_list.end(),
			[prefab_component_id](const auto& kvpair) { return kvpair.first == prefab_component_id; });
		if (prefabCompIt != component_list.end())
		{
			const PrefabComponent* prefabComp = reinterpret_cast<const PrefabComponent*>(prefabCompIt->second.get());
			m_PrefabContext.isPrefabInstance = true;
			m_PrefabContext.prefabComponent = prefabComp;

			// Load prefab data for comparison
			std::string prefabGuidStr = prefabComp->m_PrefabGuid.m_guid.to_hex();

			// Try to load prefab data from ResourceSystem
			// Note: This runs on the editor thread, ResourceSystem is accessed from engine thread
			// For now, try to get the prefab path via AssetManager and load directly
			std::string prefabPath = m_AssetManager->ResolveAssetName(prefabComp->m_PrefabGuid);

			if (!prefabPath.empty() && std::filesystem::exists(prefabPath))
			{
				try {
					PrefabData loadedData = PrefabSystem::LoadPrefabFromFile(prefabPath);
					if (loadedData.IsValid())
					{
						m_LoadedPrefabData = std::make_unique<PrefabData>(std::move(loadedData));
						m_PrefabContext.prefabData = m_LoadedPrefabData.get();
						spdlog::debug("Loaded prefab data for comparison: {}", m_LoadedPrefabData->name);
					}
				}
				catch (const std::exception& e) {
					spdlog::warn("Failed to load prefab data for comparison: {}", e.what());
				}
			}

			spdlog::debug("Rendering prefab instance with GUID: {}", prefabGuidStr);
		}
	}

	// Prefab Instance Control Panel
	if (m_PrefabContext.isPrefabInstance && m_PrefabContext.prefabComponent)
	{
		ImGui::Separator();
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.6f, 0.4f));
		if (ImGui::CollapsingHeader("Prefab Instance", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PopStyleColor();
			ImGui::Indent();

			// Display prefab GUID
			std::string prefabGuidStr = m_PrefabContext.prefabComponent->m_PrefabGuid.m_guid.to_hex();
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Prefab GUID:");
			ImGui::SameLine();
			ImGui::TextWrapped("%s", prefabGuidStr.c_str());

			ImGui::Spacing();

			// Count overrides
			int overrideCount = static_cast<int>(m_PrefabContext.prefabComponent->m_OverriddenProperties.size());
			ImGui::Text("Overrides: %d", overrideCount);

			ImGui::Spacing();

			// Apply to Prefab button
			if (overrideCount > 0)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.3f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.4f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.8f, 0.5f, 1.0f));
				if (ImGui::Button("Apply Overrides to Prefab", ImVec2(-1, 0)))
				{
					// Get prefab path from GUID via AssetManager
					std::string prefabPath = m_AssetManager->ResolveAssetName(m_PrefabContext.prefabComponent->m_PrefabGuid);

					if (prefabPath.empty())
					{
						spdlog::error("Failed to resolve prefab path from GUID");
					}
					else
					{
						engineService.ExecuteOnEngineThread([this, prefabPath]() {
							auto world = Engine::GetWorld();
							// Find the prefab instance entity
							for (auto& entity : world.get_all_entities())
							{
								if (entity.get_uid() == m_SelectedEntityID && entity.all<PrefabComponent>())
								{
									// Apply instance state to prefab file
									bool success = PrefabSystem::ApplyInstanceToPrefab(world, entity, prefabPath);

									if (success)
									{
										spdlog::info("Successfully applied overrides to prefab: {}", prefabPath);

										// Clear overrides since they're now baked into the prefab
										auto& prefabComp = entity.get<PrefabComponent>();
										prefabComp.m_OverriddenProperties.clear();

										// Optionally: sync other instances of this prefab
										// PrefabSystem::SyncPrefab(world, prefabComp.m_PrefabGuid);
									}
									else
									{
										spdlog::error("Failed to apply overrides to prefab: {}", prefabPath);
									}
									break;
								}
							}
						});
					}
				}
				ImGui::PopStyleColor(3);

				ImGui::Spacing();
			}

			// Revert all button
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.4f, 0.4f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.5f, 0.5f, 1.0f));
			if (ImGui::Button("Revert All to Prefab", ImVec2(-1, 0)))
			{
				engineService.ExecuteOnEngineThread([this]() {
					auto world = Engine::GetWorld();
					for (auto& entity : world.get_all_entities())
					{
						if (entity.get_uid() == m_SelectedEntityID)
						{
							PrefabSystem::RevertAllOverrides(world, entity);
							spdlog::info("Reverted all overrides for entity {}", m_SelectedEntityID);
							break;
						}
					}
				});
			}
			ImGui::PopStyleColor(3);

			ImGui::Unindent();
		}
		else
		{
			ImGui::PopStyleColor();
		}
		ImGui::Separator();
	}

	static const ReflectionRegistry::TypeID skip_name_component{ internal_type_map[entt::type_index<ecs::entity::entity_name_t>::value()] };
	static const ReflectionRegistry::TypeID skip_sceneid_component{ internal_type_map[entt::type_index<SceneIDComponent>::value()] };
	static const ReflectionRegistry::TypeID skip_relationship_component{ std::find_if(ReflectionRegistry::types().begin(), ReflectionRegistry::types().end(), [](auto const& kv) {return kv.second.id() == ToTypeName("Relationship"); })->first};
	ReflectionRegistry::TypeID behaviour_component{};
	auto behaviourIt = internal_type_map.find(entt::type_index<behaviour>::value());
	if (behaviourIt != internal_type_map.end())
	{
		behaviour_component = behaviourIt->second;
	}

	ReflectionRegistry::TypeID audio_component{};
	auto audioIt = internal_type_map.find(entt::type_index<AudioComponent>::value());
	if (audioIt != internal_type_map.end())
	{
		audio_component = audioIt->second;
	}

	ReflectionRegistry::TypeID rb_component{};
	auto rbIt = internal_type_map.find(entt::type_index<RigidBodyComponent>::value());
	if (rbIt != internal_type_map.end())
	{
		rb_component = rbIt->second;
	}

	ReflectionRegistry::TypeID boxCol_component{};
	auto boxIt = internal_type_map.find(entt::type_index<BoxCollider>::value());
	if (boxIt != internal_type_map.end())
	{
		boxCol_component = boxIt->second;
	}

	ReflectionRegistry::TypeID sphereCol_component{};
	auto sphereIt = internal_type_map.find(entt::type_index<SphereCollider>::value());
	if (sphereIt != internal_type_map.end())
	{
		sphereCol_component = sphereIt->second;
	}

	ReflectionRegistry::TypeID capsuleCol_component{};
	auto capsuleIt = internal_type_map.find(entt::type_index<CapsuleCollider>::value());
	if (capsuleIt != internal_type_map.end())
	{
		capsuleCol_component = capsuleIt->second;
	}

	ReflectionRegistry::TypeID material_overrides_component{};
	auto matOverridesIt = internal_type_map.find(entt::type_index<MaterialOverridesComponent>::value());
	if (matOverridesIt != internal_type_map.end())
	{
		material_overrides_component = matOverridesIt->second;
	}

	for (auto const& [type_id, uptr] : component_list) {
		if (type_id == skip_name_component || type_id == skip_sceneid_component || type_id == skip_relationship_component || type_id == prefab_component_id) {
			continue;  // Skip PrefabComponent as it's internal
		}
		if (behaviour_component && type_id == behaviour_component)
		{
			if (behaviour* behaviour_component_ptr = reinterpret_cast<behaviour*>(uptr.get()))
			{
				if (ImGui::TreeNode("Behaviour"))
				{
					Render_Behaviour_Component(*behaviour_component_ptr);
					ImGui::TreePop();
				}
			}
			continue;
		}


		entt::meta_type type = type_map[type_id];
		entt::meta_any comp = type.from_void(uptr.get());
		const char* componentLabel = "Component";
		auto& typeNames = ReflectionRegistry::TypeNames();
		if (auto itName = typeNames.find(type.id()); itName != typeNames.end())
		{
			componentLabel = itName->second.c_str();
		}

		// Check if this component has overrides in the prefab instance
		bool hasOverrides = false;
		int overrideCount = 0;
		if (m_PrefabContext.isPrefabInstance && m_PrefabContext.prefabComponent)
		{
			for (const auto& override : m_PrefabContext.prefabComponent->m_OverriddenProperties)
			{
				if (override.componentTypeHash == type_id)
				{
					hasOverrides = true;
					overrideCount++;
				}
			}
		}

		// Apply colored background for components with overrides
		if (hasOverrides)
		{
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.5f, 0.8f, 0.25f));  // Light blue background
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.6f, 0.9f, 0.35f));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.4f, 0.7f, 1.0f, 0.45f));
		}

		// Component header with override count
		std::string headerLabel = std::string(componentLabel);
		if (hasOverrides)
		{
			headerLabel += " (" + std::to_string(overrideCount) + " overrides)";
		}

		if (ImGui::TreeNodeEx(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			if (hasOverrides)
			{
				ImGui::PopStyleColor(3);  // Pop the colored background
			}

			// Show "Revert All Overrides" button for components with overrides
			if (hasOverrides)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.4f, 0.4f, 1.0f));
				if (ImGui::SmallButton("Revert All Component Overrides"))
				{
					ul.unlock();
					engineService.ExecuteOnEngineThread([this, type_id, componentLabel]() {
						auto world = Engine::GetWorld();
						for (auto& entity : world.get_all_entities()) {
							if (entity.get_uid() == m_SelectedEntityID) {
								if (entity.all<PrefabComponent>())
								{
									auto& prefabComp = entity.get<PrefabComponent>();
									// Remove all overrides for this component
									prefabComp.m_OverriddenProperties.erase(
										std::remove_if(prefabComp.m_OverriddenProperties.begin(),
											prefabComp.m_OverriddenProperties.end(),
											[type_id](const PropertyOverride& o) {
												return o.componentTypeHash == type_id;
											}),
										prefabComp.m_OverriddenProperties.end()
									);
									// Sync the entity with prefab
									PrefabSystem::SyncInstance(world, entity);
									spdlog::info("Reverted all overrides for component: {}", componentLabel);
								}
								break;
							}
						}
					});
					ul.lock();
				}
				ImGui::PopStyleColor(2);
				ImGui::Separator();
			}

			bool is_dirty = false;
			m_PrefabContext.currentComponentTypeHash = type_id;
			m_PrefabContext.currentComponentTypeName = componentLabel;
			

			if (rb_component && type_id == rb_component)
			{
				if (RigidBodyComponent* rb_comp = reinterpret_cast<RigidBodyComponent*>(uptr.get()))
				{
					is_dirty = Render_RigidBody_Component(*rb_comp);
					if (ImGui::Button("Delete Component")) {
						ul.unlock();
						engineService.delete_component(engineService.m_cont->m_snapshot_entity_handle, type_id);
						ul.lock();
					}
					if (is_dirty) {
						engineService.m_cont->m_write_back_queue.push(type_id);
					}
				}
				ImGui::TreePop();
				continue;
			}

			if (sphereCol_component && type_id == sphereCol_component)
			{
				if (SphereCollider* sphereCol_comp = reinterpret_cast<SphereCollider*>(uptr.get()))
				{
					is_dirty = Render_SphereCollider_Component(*sphereCol_comp);
					if (ImGui::Button("Delete Component")) {
						ul.unlock();
						engineService.delete_component(engineService.m_cont->m_snapshot_entity_handle, type_id);
						ul.lock();
					}
					if (is_dirty) {
						engineService.m_cont->m_write_back_queue.push(type_id);
					}
				}
				ImGui::TreePop();
				continue;
			}

			if (boxCol_component && type_id == boxCol_component)
			{
				if (BoxCollider* boxCol_comp = reinterpret_cast<BoxCollider*>(uptr.get()))
				{
					is_dirty = Render_BoxCollider_Component(*boxCol_comp);
					if (ImGui::Button("Delete Component")) {
						ul.unlock();
						engineService.delete_component(engineService.m_cont->m_snapshot_entity_handle, type_id);
						ul.lock();
					}
					if (is_dirty) {
						engineService.m_cont->m_write_back_queue.push(type_id);
					}
				}
				ImGui::TreePop();
				continue;
			}

			if (capsuleCol_component && type_id == capsuleCol_component)
			{
				if (CapsuleCollider* capsuleCol_comp = reinterpret_cast<CapsuleCollider*>(uptr.get()))
				{
					is_dirty = Render_CapsuleCollider_Component(*capsuleCol_comp);
					if (ImGui::Button("Delete Component")) {
						ul.unlock();
						engineService.delete_component(engineService.m_cont->m_snapshot_entity_handle, type_id);
						ul.lock();
					}
					if (is_dirty) {
						engineService.m_cont->m_write_back_queue.push(type_id);
					}
				}
				ImGui::TreePop();
				continue;
			}

			Render_Component_Member(comp, is_dirty);

			// Special UI section for AudioComponent playback controls
			if (audio_component && type_id == audio_component) {
				if (AudioComponent* audioComp = reinterpret_cast<AudioComponent*>(uptr.get())) {
					ImGui::Separator();
					ImGui::Text("Playback Controls");
					ImGui::BeginDisabled(!audioComp->isInitialized);

					if (ImGui::Button("Play", ImVec2(60, 0))) {
						ul.unlock();
						engineService.ExecuteOnEngineThread([entityHandle = engineService.m_cont->m_snapshot_entity_handle]() {
							ecs::entity entity{ entityHandle };
							if (entity.all<AudioComponent>()) {
								AudioComponent& audio = entity.get<AudioComponent>();
								audio.Play();
							}
						});
						ul.lock();
					}
					ImGui::SameLine();
					if (ImGui::Button("Pause", ImVec2(60, 0))) {
						ul.unlock();
						engineService.ExecuteOnEngineThread([entityHandle = engineService.m_cont->m_snapshot_entity_handle]() {
							ecs::entity entity{ entityHandle };
							if (entity.all<AudioComponent>()) {
								AudioComponent& audio = entity.get<AudioComponent>();
								if (audio.isPlaying && !audio.isPaused) {
									audio.Pause();
								} else if (audio.isPaused) {
									audio.Resume();
								}
							}
						});
						ul.lock();
					}
					ImGui::SameLine();
					if (ImGui::Button("Stop", ImVec2(60, 0))) {
						ul.unlock();
						engineService.ExecuteOnEngineThread([entityHandle = engineService.m_cont->m_snapshot_entity_handle]() {
							ecs::entity entity{ entityHandle };
							if (entity.all<AudioComponent>()) {
								AudioComponent& audio = entity.get<AudioComponent>();
								audio.Stop();
							}
						});
						ul.lock();
					}

					ImGui::EndDisabled();
					ImGui::Text("Status: %s", audioComp->isPlaying ? (audioComp->isPaused ? "Paused" : "Playing") : "Stopped");
				}
			}

			// Special UI section for MaterialOverridesComponent
			if (material_overrides_component && type_id == material_overrides_component) {
				if (MaterialOverridesComponent* matOverrides = reinterpret_cast<MaterialOverridesComponent*>(uptr.get())) {
					ImGui::Separator();
					ImGui::TextDisabled("(?)");
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Material Overrides allow per-entity customization of material properties.\n"
							"Empty maps use base material values.\n"
							"Add properties like 'u_MetallicValue' (float), 'u_AlbedoColor' (vec3), etc.");
					}
					ImGui::SameLine();
					if (ImGui::Button("Populate from Material")) {
						ul.unlock();
						engineService.ExecuteOnEngineThread([entityHandle = engineService.m_cont->m_snapshot_entity_handle]() {
							ecs::entity entity{ entityHandle };
							if (entity.all<MeshRendererComponent, MaterialOverridesComponent>()) {
								//MeshRendererComponent& meshRenderer = entity.get<MeshRendererComponent>();
								MaterialOverridesComponent& overrides = entity.get<MaterialOverridesComponent>();

								// Clear existing overrides
								overrides.ClearAll();

								// For now, populate with standard PBR properties
								// These are the common properties that most materials have
								overrides.floatOverrides["u_MetallicValue"] = 0.7f;  // Default metallic
								overrides.floatOverrides["u_RoughnessValue"] = 0.3f;  // Default roughness
								overrides.vec3Overrides["u_AlbedoColor"] = glm::vec3(0.8f, 0.7f, 0.6f);  // Default albedo

								spdlog::info("Populated MaterialOverridesComponent with standard PBR properties");
							} else {
								spdlog::warn("Entity must have both MeshRendererComponent and MaterialOverridesComponent to populate overrides");
							}
						});
						ul.lock();
					}
					ImGui::SameLine();
					ImGui::TextDisabled("Adds standard PBR properties (u_MetallicValue, u_RoughnessValue, u_AlbedoColor)");
				}
			}


			if (ImGui::Button("Delete Component")) {
				ul.unlock();
				engineService.delete_component(engineService.m_cont->m_snapshot_entity_handle, type_id);
				ul.lock();
			}
			ImGui::TreePop();
			if (is_dirty) {
				engineService.m_cont->m_write_back_queue.push(type_id);
			}
		}
		else if (hasOverrides)
		{
			// If tree node was not opened, we still need to pop the color styling
			ImGui::PopStyleColor(3);
		}
	}
}

template <typename T>
void assign_enum_helper(void* dest, const void* src) {
	*reinterpret_cast<T*>(dest) = *reinterpret_cast<const T*>(src);
};

void EditorMain::Render_Component_Member(auto& comp, bool& is_dirty)
{
	auto type = comp.type();
	auto field_table = ReflectionRegistry::GetFieldNames(type.id());

	int i{};

	for (auto [id, data] : type.data()) {
		std::string field_name{ field_table[id] };

		ImGui::PushID(i++);

		auto value = data.get(comp);
		auto meta_type = value.type();

		// === PREFAB OVERRIDE DETECTION (Per-Property) ===
		bool isPropertyOverridden = false;
		if (m_PrefabContext.isPrefabInstance && m_PrefabContext.prefabComponent)
		{
			// Check if this specific property is overridden
			for (const auto& override : m_PrefabContext.prefabComponent->m_OverriddenProperties)
			{
				if (override.componentTypeHash == m_PrefabContext.currentComponentTypeHash &&
					override.propertyPath == field_name)
				{
					isPropertyOverridden = true;
					break;
				}
			}
		}

		// Apply visual styling for overridden properties
		if (isPropertyOverridden)
		{
			// Push bold font (if available)
			// ImGui doesn't have built-in bold, so we use color instead
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.6f, 1.0f)); // Light yellow text
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.5f, 0.7f, 0.15f)); // Light blue background
		}

		// Auto-detect color properties: vec3/vec4 with "Color" or "Tint" in name
		if (glm::vec3* vec3_ptr = value.try_cast<glm::vec3>()) {
			if (field_name.find("Color") != std::string::npos ||
				field_name.find("Tint") != std::string::npos ||
				field_name.find("color") != std::string::npos ||
				field_name.find("tint") != std::string::npos) {
				// Render as RGB color picker
				if (ImGui::ColorEdit3(field_name.c_str(), &vec3_ptr->x)) {
					is_dirty = true;
				}
				ImGui::PopID();
				continue;
			}
		}
		else if (glm::vec4 * vec4_ptr = value.try_cast<glm::vec4>()) {
			if (field_name.find("Color") != std::string::npos ||
				field_name.find("Tint") != std::string::npos ||
				field_name.find("color") != std::string::npos ||
				field_name.find("tint") != std::string::npos) {
				// Render as RGBA color picker
				if (ImGui::ColorEdit4(field_name.c_str(), &vec4_ptr->x)) {
					is_dirty = true;
				}
				ImGui::PopID();
				continue;
			}
		}

		// Unity-style vec2 rendering (horizontal layout with X/Y labels)
		if (glm::vec2* vec2_ptr = value.try_cast<glm::vec2>()) {
			ImGui::Text("%s", field_name.c_str());
			ImGui::SameLine(150);  // Align input fields

			ImGui::PushItemWidth(60);  // Narrow input boxes

			// X component (red label)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
			ImGui::Text("X");
			ImGui::PopStyleColor();
			ImGui::SameLine();
			if (ImGui::DragFloat("##X", &vec2_ptr->x)) is_dirty = true;
			ImGui::SameLine();

			// Y component (green label)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
			ImGui::Text("Y");
			ImGui::PopStyleColor();
			ImGui::SameLine();
			if (ImGui::DragFloat("##Y", &vec2_ptr->y)) is_dirty = true;

			ImGui::PopItemWidth();
			ImGui::PopID();
			continue;
		}

		// Unity-style vec3 rendering (horizontal layout with X/Y/Z labels)
		// Skip if it's a color field (already handled above with ColorEdit3)
		if (glm::vec3* vec3_ptr = value.try_cast<glm::vec3>()) {
			bool isColorField = (field_name.find("Color") != std::string::npos ||
			                     field_name.find("color") != std::string::npos ||
			                     field_name.find("Tint") != std::string::npos ||
			                     field_name.find("tint") != std::string::npos);

			if (!isColorField) {
				ImGui::Text("%s", field_name.c_str());
				ImGui::SameLine(150);  // Align input fields

				ImGui::PushItemWidth(60);  // Narrow input boxes

				// X component (red label)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
				ImGui::Text("X");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				if (ImGui::DragFloat("##X", &vec3_ptr->x)) is_dirty = true;
				ImGui::SameLine();

				// Y component (green label)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
				ImGui::Text("Y");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				if (ImGui::DragFloat("##Y", &vec3_ptr->y)) is_dirty = true;
				ImGui::SameLine();

				// Z component (blue label)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 1.0f, 1.0f));
				ImGui::Text("Z");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				if (ImGui::DragFloat("##Z", &vec3_ptr->z)) is_dirty = true;

				ImGui::PopItemWidth();
				ImGui::PopID();
				continue;
			}
		}

		if (meta_type.data().begin() != meta_type.data().end()) {
			if (ImGui::CollapsingHeader(field_name.c_str())) {
				Render_Component_Member(value, is_dirty);
			}
		}
		else {
			if (meta_type.is_enum())
			{
				const void* val_ptr = value.base().data();
				void(*assignment_by_type)(void*, const void*) {nullptr};

				// Properly read enum value based on its underlying type size
				int enum_value = 0;
				if (meta_type.size_of() == sizeof(uint8_t)) {
					enum_value = *static_cast<const uint8_t*>(val_ptr);
					assignment_by_type = assign_enum_helper<uint8_t>;
				}
				else if (meta_type.size_of() == sizeof(uint16_t)) {
					enum_value = *static_cast<const uint16_t*>(val_ptr);
					assignment_by_type = assign_enum_helper<uint16_t>;
				}
				else if (meta_type.size_of() == sizeof(uint32_t)) {
					enum_value = *static_cast<const uint32_t*>(val_ptr);
					assignment_by_type = assign_enum_helper<uint32_t>;
				}
				else {
					std::cerr << "Unsupported enum underlying type size: " << meta_type.size_of() << " bytes\n";
				}

				if (ImGui::BeginCombo(field_name.c_str(), std::string(meta_type.func("to_enum_name"_tn).invoke({}, enum_value).cast<const std::string_view>()).c_str())) {
					auto names{ meta_type.func("enum_values"_tn).invoke({}).cast<std::span<const std::pair<std::string_view, std::uint32_t>>>() };
					for (auto [enum_name, enum_val] : names) {
						bool selected = (enum_val == enum_value);
						if (ImGui::Selectable(std::string(enum_name).c_str(), selected)) {
							enum_value = enum_val;
							is_dirty = true;
							assignment_by_type(const_cast<void*>(val_ptr), &enum_value);
						}
						if (selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}

			if (rp::BasicIndexedGuid* v = value.try_cast<rp::BasicIndexedGuid>()) {
				std::vector<std::string> assetnames = m_AssetManager->GetAssetTypeNames(v->m_typeindex);
				std::string currentselectionname = m_AssetManager->ResolveAssetName(*v);
				std::size_t typehash = v->m_typeindex;
				assetnames.emplace_back("");
				auto it{ std::find_if(assetnames.begin(), assetnames.end(), [currentselectionname](std::string const& a) {return currentselectionname == a; }) };
				if (it != assetnames.end()) {
					std::swap(*it, assetnames.front());
				}
				int current_item{};
				ImGui::Text(field_name.c_str());
				ImGui::SameLine(150);
				ImGui::SetNextItemWidth(-1);
				if (ImGui::Combo("##guid selector", &current_item, [](void* data, int idx, const char** out_text) {
					auto& vec = *static_cast<std::vector<std::string>*>(data);
					if (idx < 0 || idx >= vec.size()) return false;
					*out_text = vec[idx].c_str();
					return true;
					}, static_cast<void*>(&assetnames), static_cast<int>(assetnames.size()))) {
				// Check if selected item is valid and not empty (instead of checking index)
				if (current_item >= 0 && current_item < assetnames.size() && !assetnames[current_item].empty()) {
					*v = m_AssetManager->ResolveAssetGuid(assetnames[current_item]);
					v->m_typeindex = typehash;
					is_dirty = true;
				}
			}
			}

			// primitives
			else if (int* vi = value.try_cast<int>()) {
				if (ImGui::InputInt(field_name.c_str(), vi)) {
					is_dirty = true;
				}
			}
			else if (float* vf = value.try_cast<float>()) {
				if (ImGui::InputFloat(field_name.c_str(), vf)) {
					is_dirty = true;
				}
			}
			else if (double* vd = value.try_cast<double>()) {
				if (ImGui::InputDouble(field_name.c_str(), vd)) {
					is_dirty = true;
				}
			}
			else if (uint8_t* vui8 = value.try_cast<uint8_t>())
			{
				uint8_t minValue = 0;
				uint8_t maxValue = 255;

				// Special case for "layer" fields to limit max to 10
				if (field_name.find("layer") != std::string::npos)
				{
					uint8_t maxLayer = 10;
					if (ImGui::SliderScalar(field_name.c_str(), ImGuiDataType_U8, vui8, &minValue, &maxLayer, "%u")) {
						is_dirty = true;
					}
				}
				else
				{
					
					if (ImGui::SliderScalar(field_name.c_str(), ImGuiDataType_U8, vui8, &minValue, &maxValue, "%u")) {
						is_dirty = true;
					}
				}

				
			}
			else if (uint32_t* vui = value.try_cast<uint32_t>()) {
				uint32_t minValue = 0;
				uint32_t maxValue = 2147483647; // UINT32_MAX

				// With custom format
				if (ImGui::SliderScalar("My Slider", ImGuiDataType_U32, vui, &minValue, &maxValue, "%u")) {
					is_dirty = true;
				}

			}
			/*else if (std::string* vs = value.try_cast<std::string>())
				if (ImGui::InputText(field_name.c_str(), vs)) {
					is_dirty = true;
				}*/
			else if (bool* vb = value.try_cast<bool>()) {
				if (ImGui::Checkbox(field_name.c_str(), vb)) {
					is_dirty = true;
				}
			}
			// Handle unordered_map<std::string, float>
			else if (auto* map_float = value.try_cast<std::unordered_map<std::string, float>>()) {
				if (ImGui::TreeNode(field_name.c_str())) {
					if (map_float->empty()) {
						ImGui::TextDisabled("(empty)");
					}
					else {
						int idx = 0;
						std::vector<std::string> keysToDelete;
						for (auto& [key, val] : *map_float) {
							ImGui::PushID(idx++);
							if (ImGui::InputFloat(key.c_str(), &val)) {
								is_dirty = true;
							}
							ImGui::SameLine();
							if (ImGui::SmallButton("X")) {
								keysToDelete.push_back(key);
								is_dirty = true;
							}
							ImGui::PopID();
						}
						// Delete marked entries
						for (const auto& key : keysToDelete) {
							map_float->erase(key);
						}
					}

					// Add button to add new float override
					if (ImGui::Button("+ Add Float Override")) {
						ImGui::OpenPopup("AddFloatOverride");
					}
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Common float properties:\n- u_MetallicValue\n- u_RoughnessValue");
					}

					static char propertyName[128] = "";
					static float propertyValue = 0.0f;
					if (ImGui::BeginPopup("AddFloatOverride")) {
						ImGui::Text("Add Float Property Override");
						ImGui::Separator();
						ImGui::InputText("Property Name", propertyName, 128);
						ImGui::InputFloat("Value", &propertyValue);

						if (ImGui::Button("Add") && strlen(propertyName) > 0) {
							(*map_float)[std::string(propertyName)] = propertyValue;
							is_dirty = true;
							propertyName[0] = '\0';  // Clear input
							propertyValue = 0.0f;
							ImGui::CloseCurrentPopup();
						}
						ImGui::SameLine();
						if (ImGui::Button("Cancel")) {
							propertyName[0] = '\0';
							propertyValue = 0.0f;
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndPopup();
					}

					ImGui::TreePop();
				}
			}
			// Handle unordered_map<std::string, glm::vec3>
			else if (auto* map_vec3 = value.try_cast<std::unordered_map<std::string, glm::vec3>>()) {
				if (ImGui::TreeNode(field_name.c_str())) {
					if (map_vec3->empty()) {
						ImGui::TextDisabled("(empty)");
					}
					else {
						int idx = 0;
						std::vector<std::string> keysToDelete;
						for (auto& [key, val] : *map_vec3) {
							ImGui::PushID(idx++);
							// Use Uint8 flag to display as 0-255 instead of 0.0-1.0
							if (ImGui::ColorEdit3(key.c_str(), &val.x, ImGuiColorEditFlags_Uint8)) {
								is_dirty = true;
							}
							ImGui::SameLine();
							if (ImGui::SmallButton("X")) {
								keysToDelete.push_back(key);
								is_dirty = true;
							}
							ImGui::PopID();
						}
						// Delete marked entries
						for (const auto& key : keysToDelete) {
							map_vec3->erase(key);
						}
					}

					// Add button to add new vec3 override
					if (ImGui::Button("+ Add Vec3 Override")) {
						ImGui::OpenPopup("AddVec3Override");
					}
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Common vec3 properties:\n- u_AlbedoColor\n- u_EmissiveColor");
					}

					static char propertyNameVec3[128] = "";
					static glm::vec3 propertyValueVec3(0.0f);
					if (ImGui::BeginPopup("AddVec3Override")) {
						ImGui::Text("Add Vec3 Property Override");
						ImGui::Separator();
						ImGui::InputText("Property Name", propertyNameVec3, 128);
						ImGui::ColorEdit3("Value", &propertyValueVec3.x, ImGuiColorEditFlags_Uint8);

						if (ImGui::Button("Add") && strlen(propertyNameVec3) > 0) {
							(*map_vec3)[std::string(propertyNameVec3)] = propertyValueVec3;
							is_dirty = true;
							propertyNameVec3[0] = '\0';  // Clear input
							propertyValueVec3 = glm::vec3(0.0f);
							ImGui::CloseCurrentPopup();
						}
						ImGui::SameLine();
						if (ImGui::Button("Cancel")) {
							propertyNameVec3[0] = '\0';
							propertyValueVec3 = glm::vec3(0.0f);
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndPopup();
					}

					ImGui::TreePop();
				}
			}
			// Handle unordered_map<std::string, glm::vec4>
			else if (auto* map_vec4 = value.try_cast<std::unordered_map<std::string, glm::vec4>>()) {
				if (ImGui::TreeNode(field_name.c_str())) {
					if (map_vec4->empty()) {
						ImGui::TextDisabled("(empty)");
					}
					else {
						int idx = 0;
						std::vector<std::string> keysToDelete;
						for (auto& [key, val] : *map_vec4) {
							ImGui::PushID(idx++);
							// Use Uint8 flag to display as 0-255 instead of 0.0-1.0
							if (ImGui::ColorEdit4(key.c_str(), &val.x, ImGuiColorEditFlags_Uint8)) {
								is_dirty = true;
							}
							ImGui::SameLine();
							if (ImGui::SmallButton("X")) {
								keysToDelete.push_back(key);
								is_dirty = true;
							}
							ImGui::PopID();
						}
						// Delete marked entries
						for (const auto& key : keysToDelete) {
							map_vec4->erase(key);
						}
					}

					// Add button to add new vec4 override
					if (ImGui::Button("+ Add Vec4 Override")) {
						ImGui::OpenPopup("AddVec4Override");
					}
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Vec4 properties typically include alpha channel\n- u_TintColor (RGBA)");
					}

					static char propertyNameVec4[128] = "";
					static glm::vec4 propertyValueVec4(0.0f);
					if (ImGui::BeginPopup("AddVec4Override")) {
						ImGui::Text("Add Vec4 Property Override");
						ImGui::Separator();
						ImGui::InputText("Property Name", propertyNameVec4, 128);
						ImGui::ColorEdit4("Value", &propertyValueVec4.x, ImGuiColorEditFlags_Uint8);

						if (ImGui::Button("Add") && strlen(propertyNameVec4) > 0) {
							(*map_vec4)[std::string(propertyNameVec4)] = propertyValueVec4;
							is_dirty = true;
							propertyNameVec4[0] = '\0';  // Clear input
							propertyValueVec4 = glm::vec4(0.0f);
							ImGui::CloseCurrentPopup();
						}
						ImGui::SameLine();
						if (ImGui::Button("Cancel")) {
							propertyNameVec4[0] = '\0';
							propertyValueVec4 = glm::vec4(0.0f);
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndPopup();
					}

					ImGui::TreePop();
				}
			}
			// Handle unordered_map<std::string, glm::mat4>
			else if (auto* map_mat4 = value.try_cast<std::unordered_map<std::string, glm::mat4>>()) {
				if (ImGui::TreeNode(field_name.c_str())) {
					if (map_mat4->empty()) {
						ImGui::TextDisabled("(empty)");
					}
					else {
						int idx = 0;
						std::vector<std::string> keysToDelete;
						for (auto& [key, val] : *map_mat4) {
							ImGui::PushID(idx++);
							ImGui::Text("%s: [mat4 - not editable]", key.c_str());
							ImGui::SameLine();
							if (ImGui::SmallButton("X")) {
								keysToDelete.push_back(key);
								is_dirty = true;
							}
							ImGui::PopID();
						}
						// Delete marked entries
						for (const auto& key : keysToDelete) {
							map_mat4->erase(key);
						}
					}

					// Note: Mat4 addition not implemented due to complexity
					ImGui::TextDisabled("Note: Use code to add mat4 overrides");

					ImGui::TreePop();
				}
			}
			// Handle unordered_map<std::string, Resource::Guid>
			else if (auto* map_guid = value.try_cast<std::unordered_map<std::string, rp::Guid>>()) {
				if (ImGui::TreeNode(field_name.c_str())) {
					if (map_guid->empty()) {
						ImGui::TextDisabled("(empty)");
					}
					else {
						for (auto& [key, val] : *map_guid) {
							ImGui::Text("%s: %s", key.c_str(), val.to_hex().c_str());
						}
					}
					ImGui::TreePop();
				}
			}
			else if (auto* vec_guid = value.try_cast<std::vector<rp::BasicIndexedGuid>>()) {
				if (ImGui::TreeNode(field_name.c_str())) {
					if (vec_guid->empty()) {
						ImGui::TextDisabled("(empty)");
					}
					else {
						int idx{};
						for (auto& guid : *vec_guid) {
							ImGui::PushID(idx++);
							std::vector<std::string> assetnames = m_AssetManager->GetAssetTypeNames(guid.m_typeindex);
							std::string currentselectionname = m_AssetManager->ResolveAssetName(guid);
							std::size_t typehash = guid.m_typeindex;
							assetnames.emplace_back("");
							auto it{ std::find_if(assetnames.begin(), assetnames.end(), [currentselectionname](std::string const& a) {return currentselectionname == a; }) };
							if (it != assetnames.end()) {
								std::swap(*it, assetnames.front());
							}
							int current_item{};
							ImGui::Text(field_name.c_str());
							ImGui::SameLine(150);
							ImGui::SetNextItemWidth(-1);
							if (ImGui::Combo("##guid selector", &current_item, [](void* data, int idx, const char** out_text) {
								auto& vec = *static_cast<std::vector<std::string>*>(data);
								if (idx < 0 || idx >= vec.size()) return false;
								*out_text = vec[idx].c_str();
								return true;
								}, static_cast<void*>(&assetnames), static_cast<int>(assetnames.size()))) {
								// Check if selected item is valid and not empty (instead of checking index)
								if (current_item >= 0 && current_item < assetnames.size() && !assetnames[current_item].empty()) {
									guid = m_AssetManager->ResolveAssetGuid(assetnames[current_item]);
									guid.m_typeindex = typehash;
									is_dirty = true;
								}
							}
							ImGui::PopID();
						}
					}
					ImGui::TreePop();
				}
				}
			else if (auto* map_name_guid = value.try_cast<std::unordered_map<std::string, rp::BasicIndexedGuid>>()) {
				if (ImGui::TreeNode(field_name.c_str())) {
					if (map_name_guid->empty()) {
						ImGui::TextDisabled("(empty)");
					}
					else {
						int idx{};
						for (auto& [name, guid] : *map_name_guid) {
							ImGui::PushID(idx++);
							std::vector<std::string> assetnames = m_AssetManager->GetAssetTypeNames(guid.m_typeindex);
							std::string currentselectionname = m_AssetManager->ResolveAssetName(guid);
							std::size_t typehash = guid.m_typeindex;
							assetnames.emplace_back("");
							auto it{ std::find_if(assetnames.begin(), assetnames.end(), [currentselectionname](std::string const& a) {return currentselectionname == a; }) };
							if (it != assetnames.end()) {
								std::swap(*it, assetnames.front());
							}
							int current_item{};
							ImGui::Text(name.c_str());
							ImGui::SameLine(150);
							ImGui::SetNextItemWidth(-1);
							if (ImGui::Combo("##guid selector", &current_item, [](void* data, int idx, const char** out_text) {
								auto& vec = *static_cast<std::vector<std::string>*>(data);
								if (idx < 0 || idx >= vec.size()) return false;
								*out_text = vec[idx].c_str();
								return true;
								}, static_cast<void*>(&assetnames), static_cast<int>(assetnames.size()))) {
								// Check if selected item is valid and not empty (instead of checking index)
								if (current_item >= 0 && current_item < assetnames.size() && !assetnames[current_item].empty()) {
									guid = m_AssetManager->ResolveAssetGuid(assetnames[current_item]);
									guid.m_typeindex = typehash;
									is_dirty = true;
								}
							}
							ImGui::PopID();
						}
					}
					ImGui::TreePop();
				}
			}
		}

		// === PREFAB OVERRIDE: Revert Button (Per-Property) ===
		if (isPropertyOverridden)
		{
			// Pop the style colors we pushed earlier
			ImGui::PopStyleColor(2);

			// Add small revert button on the same line
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.3f, 0.8f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.4f, 0.4f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.5f, 0.5f, 1.0f));

			if (ImGui::SmallButton("Revert"))
			{
				// Revert this specific property override
				engineService.ExecuteOnEngineThread([this, field_name]() {
					auto world = Engine::GetWorld();
					for (auto& entity : world.get_all_entities())
					{
						if (entity.get_uid() == m_SelectedEntityID && entity.all<PrefabComponent>())
						{
							PrefabSystem::RevertOverride(
								world,
								entity,
								m_PrefabContext.currentComponentTypeHash,
								field_name
							);
							spdlog::info("Reverted property override: {}.{}",
								m_PrefabContext.currentComponentTypeName, field_name);
							break;
						}
					}
				});
			}

			ImGui::PopStyleColor(3);

			// Tooltip for the revert button
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Revert '%s' to prefab value", field_name.c_str());
			}
		}

		ImGui::PopID();
	}
}

void EditorMain::Render_Add_Component_Menu()
{
	auto& reflectible_component_list = engineService.get_reflectible_component_id_name_list();
	static const ReflectionRegistry::TypeID skip_name_component{ ReflectionRegistry::InternalID()[entt::type_index<ecs::entity::entity_name_t>::value()] };
	static const ReflectionRegistry::TypeID skip_sceneid_component{ ReflectionRegistry::InternalID()[entt::type_index<SceneIDComponent>::value()] };
	static const ReflectionRegistry::TypeID skip_relationship_component{ std::find_if(ReflectionRegistry::types().begin(), ReflectionRegistry::types().end(), [](auto const& kv) {return kv.second.id() == ToTypeName("Relationship"); })->first };
	for (auto const& [type_id, type_name] : reflectible_component_list) {
		if (type_id == skip_name_component || type_id == skip_sceneid_component || type_id == skip_relationship_component) {
			continue;
		}
		if (ImGui::MenuItem(type_name.c_str())) {
			/*if (type_name == "Behaviour")
			{
				ecs::entity entity{ static_cast<std::uint32_t>(Engine::GetWorld()), m_SelectedEntityID };
				entity.add<behaviour>();
			}*/
			engineService.add_component(engineService.m_cont->m_snapshot_entity_handle, type_id);
		}
	}
}

void EditorMain::Render_Behaviour_Component(behaviour& component)
{
	auto localSelectedEntityID = m_SelectedEntityID;


	bool addScript = ImGui::Button("Add Script");

	if (addScript)
	{
		ImGui::OpenPopup("Add Script Popup");

	}
	Add_Script_Menu();

	if (component.classesName.empty())
	{
		ImGui::Text("No scripts attached.");
		return;
	}

	//auto& entityManager = MonoEntityManager::GetInstance();
	//auto& typeRegistry = entityManager.GetTypeRegistry();


	for (std::size_t i = 0; i < component.classesName.size(); ++i)
	{
		ImGui::Separator();
		const std::string& fullName = component.classesName[i];
		std::string label = fullName.empty() ? std::string("<Unnamed Script>") : fullName;
		label.append("##BehaviourScript");
		label.append(std::to_string(i));
		rp::Guid scriptGuid = (i < component.scriptIDs.size()) ? component.scriptIDs[i] : rp::Guid{};
		if (!scriptGuid)
		{
			continue;
		}

		if (ImGui::TreeNode(label.c_str()))
		{
			MonoImGuiRenderer::RenderBehaviourFields(fullName, scriptGuid);
			ImGui::TreePop();
		}


		// Pad the button downward living a bit of space from the tree nod
		// to the separator
		ImVec2 padding = { 0.0f, 10.0f };
		ImGui::Dummy(padding);
		std::string const id = "REMOVE_SCRIPT_BTN_" + scriptGuid.to_hex();
		ImGui::PushID(id.c_str());
		if (ImGui::Button("Remove Script"))
		{
			spdlog::info("Remove Script is clicked");
			std::cout << "classesName size=" << component.classesName.size()
				<< ", scriptIDs size=" << component.scriptIDs.size()
				<< ", i=" << i << std::endl;

			auto RemoveScripts = [i, localSelectedEntityID]()
			{
				ecs::entity entity{ static_cast<std::uint32_t>(Engine::GetWorld()), localSelectedEntityID };
				behaviour& behaviour_component = entity.get<behaviour>();
				if (i < behaviour_component.classesName.size())
				{
					behaviour_component.classesName.erase(behaviour_component.classesName.begin() + i);
				}
				if (i < behaviour_component.scriptIDs.size())
				{
					behaviour_component.scriptIDs.erase(behaviour_component.scriptIDs.begin() + i);
				}

			};
			engineService.m_cont->m_command_queue.push(RemoveScripts);
			ImGui::PopID();
			break;
		}
		ImGui::PopID();
		ImGui::Separator();
	}

}

void EditorMain::Add_Script_Menu()
{
	if (ImGui::BeginPopup("Add Script Popup"))
	{
		std::vector<std::string> scriptClasses{};
		auto gameAssemblyID = MonoEntityManager::GetInstance().GetPrimaryAssembly();
		MonoReflectionRegistry::Instance().VisitAssemblies([gameAssemblyID, &scriptClasses](AssemblyNode const& node)
		{
			if (node.name == "BasilEngine") return;
			for (auto const& [className, classNodePtr] : node.classes)
			{
				scriptClasses.push_back(className);

			}
		});

		// List all script classes
		for (const auto& className : scriptClasses)
		{
			if (ImGui::MenuItem(className.c_str()))
			{
				auto add_script_to_behaviour = [className, localSelectedEntityID = m_SelectedEntityID]()
				{
					ecs::entity entity{ static_cast<std::uint32_t>(Engine::GetWorld()), localSelectedEntityID };
					behaviour& behaviour_component = entity.get<behaviour>();
					behaviour_component.classesName.push_back(className);
					ecs::world temp{ Engine::GetWorld() };
					BehaviourSystem::Instance().AddScriptToEntityComponent(entity, temp, className.c_str());
				};
				engineService.m_cont->m_command_queue.push(add_script_to_behaviour);
			}
		}

		ImGui::EndPopup();
	}
}

bool EditorMain::Render_RigidBody_Component(RigidBodyComponent& rb) {
	bool changed = false;

	// ===== Debug Info Section =====
	ImGui::SeparatorText("Debug Info");
	ImGui::PushStyleColor(ImGuiCol_Text, rb.isDirty ? ImVec4(1.0f, 0.7f, 0.3f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
	ImGui::Checkbox("Is Dirty (needs sync)", &rb.isDirty);
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("When true, component will sync to physics on next frame");
	}

	if (ImGui::Checkbox("Is Active", &rb.isActive)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Whether this body participates in simulation");
	}

	// ===== Motion Properties Section =====
	ImGui::SeparatorText("Motion Properties");

	// Motion Type
	const char* motionTypes[] = { "Static", "Dynamic", "Kinematic" };
	int currentMotionType = static_cast<int>(rb.motionType);
	if (ImGui::Combo("Motion Type", &currentMotionType, motionTypes, 3)) {
		rb.motionType = static_cast<RigidBodyComponent::MotionType>(currentMotionType);
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Static: Doesn't move\nDynamic: Fully simulated\nKinematic: Controlled by code");
	}

	// Mass (only for dynamic)
	if (rb.motionType == RigidBodyComponent::MotionType::Dynamic) {
		if (ImGui::DragFloat("Mass (kg)", &rb.mass, 0.1f, 0.001f, 10000.0f)) {
			changed = true;
		}
	}
	else {
		ImGui::BeginDisabled();
		ImGui::DragFloat("Mass (kg)", &rb.mass, 0.1f, 0.001f, 10000.0f);
		ImGui::EndDisabled();
	}

	// Collision Detection Mode
	const char* collisionModes[] = { "Discrete", "Continuous", "Continuous Dynamic", "Continuous Speculative" };
	int currentCollisionMode = static_cast<int>(rb.collisionDetection);
	if (ImGui::Combo("Collision Detection", &currentCollisionMode, collisionModes, 4)) {
		rb.collisionDetection = static_cast<RigidBodyComponent::CollisionDetectionMode>(currentCollisionMode);
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Discrete: Fast but can miss fast collisions\nContinuous: Slower but catches everything");
	}

	// Interpolation Mode
	const char* interpolationModes[] = { "None", "Interpolate", "Extrapolate" };
	int currentInterpolation = static_cast<int>(rb.interpolation);
	if (ImGui::Combo("Interpolation", &currentInterpolation, interpolationModes, 3)) {
		rb.interpolation = static_cast<RigidBodyComponent::InterpolationMode>(currentInterpolation);
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Smoothing for rendering:\nNone: Can look jittery\nInterpolate: Smooth\nExtrapolate: Predictive");
	}

	// ===== Physics Properties Section =====
	ImGui::SeparatorText("Physics Properties");

	// Gravity
	if (ImGui::Checkbox("Use Gravity", &rb.useGravity)) {
		changed = true;
	}

	if (ImGui::DragFloat("Gravity Factor", &rb.gravityFactor, 0.1f, -50.0f, 50.0f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Multiplier for gravity (default: 1.00 m/s�)");
	}

	// Friction
	if (ImGui::DragFloat("Friction", &rb.friction, 0.01f, 0.0f, 1.0f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Surface friction (0 = ice, 1 = rubber)");
	}

	// ===== Damping Section =====
	ImGui::SeparatorText("Damping");

	// Linear Drag
	if (ImGui::DragFloat("Drag (Linear)", &rb.drag, 0.01f, 0.0f, 10.0f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Air resistance:\n0.0 = space (no resistance)\n0.5 = water\n5.0 = honey");
	}

	// Linear Damping
	if (ImGui::DragFloat("Linear Damping", &rb.linearDamping, 0.01f, 0.0f, 1.0f)) {
		changed = true;
	}

	if (ImGui::DragFloat("Angular Damping", &rb.angularDrag, 0.01f, 0.0f, 1.0f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Prevents objects from spinning forever");
	}

	// ===== Constraints Section =====
	ImGui::SeparatorText("Constraints");

	ImGui::Text("Freeze Position:");
	if (ImGui::Checkbox("X##FreezePos", &rb.freezePositionX)) changed = true;
	ImGui::SameLine();
	if (ImGui::Checkbox("Y##FreezePos", &rb.freezePositionY)) changed = true;
	ImGui::SameLine();
	if (ImGui::Checkbox("Z##FreezePos", &rb.freezePositionZ)) changed = true;
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Locks position on specific axes\nAlso zeros velocity on those axes to prevent force accumulation");
	}

	ImGui::Text("Freeze Rotation:");
	if (ImGui::Checkbox("X##FreezeRot", &rb.freezeRotationX)) changed = true;
	ImGui::SameLine();
	if (ImGui::Checkbox("Y##FreezeRot", &rb.freezeRotationY)) changed = true;
	ImGui::SameLine();
	if (ImGui::Checkbox("Z##FreezeRot", &rb.freezeRotationZ)) changed = true;
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Locks rotation around specific axes\nAlso zeros angular velocity on those axes to prevent torque accumulation");
	}

	ImGui::Text("Freeze Linear Velocity:");
	if (ImGui::Checkbox("X##FreezeLinVel", &rb.freezeLinearVelocityX)) changed = true;
	ImGui::SameLine();
	if (ImGui::Checkbox("Y##FreezeLinVel", &rb.freezeLinearVelocityY)) changed = true;
	ImGui::SameLine();
	if (ImGui::Checkbox("Z##FreezeLinVel", &rb.freezeLinearVelocityZ)) changed = true;
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Additional velocity constraints (independent of position freeze)\nUseful for advanced control scenarios");
	}

	ImGui::Text("Freeze Angular Velocity:");
	if (ImGui::Checkbox("X##FreezeAngVel", &rb.freezeAngularVelocityX)) changed = true;
	ImGui::SameLine();
	if (ImGui::Checkbox("Y##FreezeAngVel", &rb.freezeAngularVelocityY)) changed = true;
	ImGui::SameLine();
	if (ImGui::Checkbox("Z##FreezeAngVel", &rb.freezeAngularVelocityZ)) changed = true;
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Additional angular velocity constraints (independent of rotation freeze)\nUseful for advanced control scenarios");
	}

	// ===== Advanced Section =====
	if (ImGui::TreeNode("Advanced")) {
		// Center of Mass
		if (ImGui::DragFloat3("Center of Mass", &rb.centerOfMass.x, 0.01f)) {
			changed = true;
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Local space offset for center of mass");
		}

		ImGui::TreePop();
	}

	// ===== Runtime State (Read-Only) =====
	ImGui::SeparatorText("Runtime State (Read-Only)");

	ImGui::BeginDisabled();
	ImGui::DragFloat3("Linear Velocity", &rb.linearVelocity.x, 0.0f);
	ImGui::DragFloat3("Angular Velocity", &rb.angularVelocity.x, 0.0f);
	ImGui::EndDisabled();

	float linearSpeed = glm::length(rb.linearVelocity);
	float angularSpeed = glm::length(rb.angularVelocity);
	ImGui::Text("Speed: %.2f m/s | Angular: %.2f rad/s", linearSpeed, angularSpeed);

	// Mark dirty if any change occurred
	if (changed) {
		rb.isDirty = true;
		spdlog::debug("EditorMain: Marked RigidBody as dirty for entity {}", m_SelectedEntityID);
	}
	return changed;
}

bool EditorMain::Render_BoxCollider_Component(BoxCollider& collider) {
	bool changed = false;

	// ===== Debug Info Section =====
	ImGui::SeparatorText("Debug Info");
	ImGui::PushStyleColor(ImGuiCol_Text, collider.isDirty ? ImVec4(1.0f, 0.7f, 0.3f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
	ImGui::Checkbox("Is Dirty (needs sync)", &collider.isDirty);
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("When true, collider shape will be recreated on next frame");
	}

	// Collision tracking (read-only)
	ImGui::PushStyleColor(ImGuiCol_Text, collider.isColliding ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
	ImGui::Text("Colliding: %s", collider.isColliding ? "YES" : "NO");
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Updated by physics system - shows if currently colliding with any object");
	}

	ImGui::Text("Collision Count: %d", collider.collisionCount);
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Number of active collisions/triggers with this collider");
	}

	// Show list of colliding entities
	if (collider.collisionCount > 0 && ImGui::TreeNode("Colliding With:")) {
		for (const auto& entity : collider.collidingWith) {
			ImGui::BulletText("%s (ID: %u)", entity.name().c_str(), entity.get_uid());
		}
		ImGui::TreePop();
	}

	// ===== Collider Type =====
	ImGui::SeparatorText("Collider Type");
	ImGui::Text("Box Collider");

	// ===== Trigger Settings =====
	ImGui::SeparatorText("Trigger Settings");
	if (ImGui::Checkbox("Is Trigger", &collider.isTrigger)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Triggers don't cause physical collisions but fire OnTrigger events");
	}

	// ===== Shape Properties =====
	ImGui::SeparatorText("Shape Properties");
	if (ImGui::DragFloat3("Size", &collider.size.x, 0.1f, 0.001f, 1000.0f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Full dimensions of the box (not half-extents)");
	}

	if (ImGui::DragFloat3("Center Offset", &collider.center.x, 0.01f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Local space offset from entity position");
	}

	// ===== Physics Material =====
	ImGui::SeparatorText("Physics Material");
	if (ImGui::DragFloat("Friction", &collider.friction, 0.01f, 0.0f, 1.0f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Surface friction (0 = ice, 1 = rubber)");
	}

	if (ImGui::DragFloat("Restitution", &collider.restitution, 0.01f, 0.0f, 1.0f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Bounciness (0 = no bounce, 1 = perfect bounce)");
	}

	if (ImGui::DragFloat("Density", &collider.density, 0.01f, 0.01f, 100.0f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Mass per unit volume (for auto mass calculation)");
	}

	// Mark dirty if any change occurred
	if (changed) {
		collider.isDirty = true;
		spdlog::debug("EditorMain: Marked BoxCollider as dirty for entity {}", m_SelectedEntityID);
	}

	return changed;
}

bool EditorMain::Render_SphereCollider_Component(SphereCollider& collider) {
	bool changed = false;

	// ===== Debug Info Section =====
	ImGui::SeparatorText("Debug Info");
	ImGui::PushStyleColor(ImGuiCol_Text, collider.isDirty ? ImVec4(1.0f, 0.7f, 0.3f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
	ImGui::Checkbox("Is Dirty (needs sync)", &collider.isDirty);
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("When true, collider shape will be recreated on next frame");
	}

	// Collision tracking (read-only)
	ImGui::PushStyleColor(ImGuiCol_Text, collider.isColliding ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
	ImGui::Text("Colliding: %s", collider.isColliding ? "YES" : "NO");
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Updated by physics system - shows if currently colliding with any object");
	}

	ImGui::Text("Collision Count: %d", collider.collisionCount);
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Number of active collisions/triggers with this collider");
	}

	// Show list of colliding entities
	if (collider.collisionCount > 0 && ImGui::TreeNode("Colliding With:")) {
		for (const auto& entity : collider.collidingWith) {
			ImGui::BulletText("%s (ID: %u)", entity.name().c_str(), entity.get_uid());
		}
		ImGui::TreePop();
	}

	// ===== Collider Type =====
	ImGui::SeparatorText("Collider Type");
	ImGui::Text("Sphere Collider");

	// ===== Trigger Settings =====
	ImGui::SeparatorText("Trigger Settings");
	if (ImGui::Checkbox("Is Trigger", &collider.isTrigger)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Triggers don't cause physical collisions but fire OnTrigger events");
	}

	// ===== Shape Properties =====
	ImGui::SeparatorText("Shape Properties");
	if (ImGui::DragFloat("Radius", &collider.radius, 0.01f, 0.001f, 1000.0f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Radius of the sphere");
	}

	if (ImGui::DragFloat3("Center Offset", &collider.center.x, 0.01f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Local space offset from entity position");
	}

	// ===== Physics Material =====
	ImGui::SeparatorText("Physics Material");
	if (ImGui::DragFloat("Friction", &collider.friction, 0.01f, 0.0f, 1.0f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Surface friction (0 = ice, 1 = rubber)");
	}

	if (ImGui::DragFloat("Restitution", &collider.restitution, 0.01f, 0.0f, 1.0f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Bounciness (0 = no bounce, 1 = perfect bounce)");
	}

	if (ImGui::DragFloat("Density", &collider.density, 0.01f, 0.01f, 100.0f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Mass per unit volume (for auto mass calculation)");
	}

	// Mark dirty if any change occurred
	if (changed) {
		collider.isDirty = true;
		spdlog::debug("EditorMain: Marked SphereCollider as dirty for entity {}", m_SelectedEntityID);
	}

	return changed;
}

bool EditorMain::Render_CapsuleCollider_Component(CapsuleCollider& collider) {
	bool changed = false;

	// ===== Debug Info Section =====
	ImGui::SeparatorText("Debug Info");
	ImGui::PushStyleColor(ImGuiCol_Text, collider.isDirty ? ImVec4(1.0f, 0.7f, 0.3f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
	ImGui::Checkbox("Is Dirty (needs sync)", &collider.isDirty);
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("When true, collider shape will be recreated on next frame");
	}

	// Collision tracking (read-only)
	ImGui::PushStyleColor(ImGuiCol_Text, collider.isColliding ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
	ImGui::Text("Colliding: %s", collider.isColliding ? "YES" : "NO");
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Updated by physics system - shows if currently colliding with any object");
	}

	ImGui::Text("Collision Count: %d", collider.collisionCount);
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Number of active collisions/triggers with this collider");
	}

	// Show list of colliding entities
	if (collider.collisionCount > 0 && ImGui::TreeNode("Colliding With:")) {
		for (const auto& entity : collider.collidingWith) {
			ImGui::BulletText("%s (ID: %u)", entity.name().c_str(), entity.get_uid());
		}
		ImGui::TreePop();
	}

	// ===== Collider Type =====
	ImGui::SeparatorText("Collider Type");
	ImGui::Text("Capsule Collider");

	// ===== Trigger Settings =====
	ImGui::SeparatorText("Trigger Settings");
	if (ImGui::Checkbox("Is Trigger", &collider.isTrigger)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Triggers don't cause physical collisions but fire OnTrigger events");
	}

	// ===== Shape Properties =====
	ImGui::SeparatorText("Shape Properties");

	// Direction
	const char* directions[] = { "X Axis", "Y Axis", "Z Axis" };
	int currentDirection = static_cast<int>(collider.GetDirection());
	if (ImGui::Combo("Direction", &currentDirection, directions, 3)) {
		collider.SetDirection(static_cast<CapsuleCollider::Direction>(currentDirection));
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Axis along which the capsule is oriented");
	}

	// Radius
	float radius = collider.GetRadius();
	if (ImGui::DragFloat("Radius", &radius, 0.01f, 0.001f, 1000.0f)) {
		collider.SetRadius(radius);
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Radius of the capsule cylinder and end caps");
	}
	
	// Height
	float colheight = collider.GetHeight();
	if (ImGui::DragFloat("Height", &colheight, 0.01f, 0.001f, 1000.0f)) {
		collider.SetHeight(colheight);
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Height of the capsule (including end caps)");
	}

	if (ImGui::DragFloat3("Center Offset", &collider.center.x, 0.01f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Local space offset from entity position");
	}

	// ===== Physics Material =====
	ImGui::SeparatorText("Physics Material");
	if (ImGui::DragFloat("Friction", &collider.friction, 0.01f, 0.0f, 1.0f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Surface friction (0 = ice, 1 = rubber)");
	}

	if (ImGui::DragFloat("Restitution", &collider.restitution, 0.01f, 0.0f, 1.0f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Bounciness (0 = no bounce, 1 = perfect bounce)");
	}

	if (ImGui::DragFloat("Density", &collider.density, 0.01f, 0.01f, 100.0f)) {
		changed = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Mass per unit volume (for auto mass calculation)");
	}

	// Mark dirty if any change occurred
	if (changed) {
		collider.isDirty = true;
		spdlog::debug("EditorMain: Marked CapsuleCollider as dirty for entity {}", m_SelectedEntityID);
	}

	return changed;
}


/*
By Default the physics is disabled, only collision is checked and displayed, but no resolution should happen, logic should also be disabled
When we are playing, phyiscs and logic should be enabled fully.
When Paused, phyics and logic should be paused.
when Stepped, phyics and logic is stepped by one
*/
void EditorMain::Render_StartStop()
{
	// Toolbar background
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.157f, 0.157f, 0.157f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));

	ImGui::BeginChild("##StartStop", ImVec2(0, 40), true, ImGuiWindowFlags_NoScrollbar);

	// Center the buttons
	float windowWidth = ImGui::GetContentRegionAvail().x;
	float buttonWidth = 60.0f; // Square buttons like Unity
	float spacing = 4.0f;
	float totalWidth = (buttonWidth * 3) + (spacing * 2);
	float offsetX = (windowWidth - totalWidth) * 0.5f;

	if (offsetX > 0)
		ImGui::SetCursorPosX(offsetX);

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0));

	// Play Button
	if (isPlaying)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.337f, 0.612f, 0.839f, 1.0f)); // Unity blue when active
	else
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.235f, 0.235f, 0.235f, 1.0f));

	if (ImGui::Button(isPlaying ? "Stop" : "Play", ImVec2(buttonWidth, 28)))
	{
		isPlaying = !isPlaying;
		if (isPlaying) // Starts game
		{
			
			SaveScene("tmp.yaml");
			engineService.ExecuteOnEngineThread([&]() {
				PhysicsSystem::Instance().isActive = true;
				PhysicsSystem::Instance().DisableObservers();
				BehaviourSystem::Instance().isActive = true;
				spdlog::info("Physics Active");


				});

			// No longer need to switch cameras - dual rendering handles both viewports
		}
		else // Stops Game
		{
			engineService.ExecuteOnEngineThread([]() {
				PhysicsSystem::Instance().Reset();
				PhysicsSystem::Instance().isActive = false;
				BehaviourSystem::Instance().isActive = false;
			});
			LoadScene("tmp.yaml");
			engineService.ExecuteOnEngineThread([]() {
				PhysicsSystem::Instance().EnableObservers();
				PhysicsSystem::Instance().CreateAllBodiesForLoadedScene();
				spdlog::info("Physics Off");
				});

			// No longer need to switch cameras - dual rendering handles both viewports
			isPaused = false; // Resets paused game as we are stopping
		}

	}
	ImGui::PopStyleColor();

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(isPlaying ? "Stop" : "Play");

	ImGui::SameLine();

	// Pause Button
	if (!isPlaying)
		ImGui::BeginDisabled();

	if (isPaused)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
	else
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.235f, 0.235f, 0.235f, 1.0f));

	if (ImGui::Button("Pause", ImVec2(buttonWidth, 28)))
	{
		isPaused = !isPaused;
		//OnPauseToggle();
	}
	ImGui::PopStyleColor();

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(isPaused ? "Resume" : "Pause");

	if (!isPlaying)
		ImGui::EndDisabled();

	ImGui::SameLine();

	// Step Button
	if (!isPaused)
		ImGui::BeginDisabled();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.235f, 0.235f, 0.235f, 1.0f));
	if (ImGui::Button("Step", ImVec2(buttonWidth, 28)))
	{
		//OnStep();
	}
	ImGui::PopStyleColor();

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Step Frame");

	if (!isPaused)
		ImGui::EndDisabled();

	ImGui::PopStyleVar(2);

	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

}

void EditorMain::Render_MenuBar()
{
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("File"))
	{
		ImGui::MenuItem("New Project", "Ctrl+Alt+N");
		ImGui::MenuItem("Open Project", "Ctrl+Alt+O");
		ImGui::MenuItem("Save Project", "Ctrl+Alt+S");
		ImGui::MenuItem("Save Project As", "Ctrl+Alt+Shift+S");
		ImGui::Separator();
		if (ImGui::MenuItem("Exit", "Alt+F4")) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Scene"))
	{
		if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
			NewScene();
		}
		if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
		{
			std::string path{};
			if (fileService.OpenFileDialog(Editor::GetInstance().GetConfig().project_workingDir.c_str(), path, FileService::FILE_TYPE_LIST{ {L"Scene Files", L"*.scene"} }))
			{
				LoadScene(path.c_str());
				glfwSetWindowTitle(window, (Editor::GetInstance().GetConfig().workspace_name + " | " + std::filesystem::path{ path }.filename().string()).c_str());


			}
		}
		if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
		{

			std::string currentPath = Editor::GetInstance().GetConfig().project_workingDir;
			if (currentPath.empty()) {
				spdlog::error("No project loaded, cannot save scene!");
				return;
			}
			std::string scenePath{};
			fileService.SaveFileDialog(Editor::GetInstance().GetConfig().project_workingDir.c_str(), scenePath, FileService::FILE_TYPE_LIST{ {L"Scene Files", L"*.scene"} });
			if (!scenePath.empty()) {

				SaveScene(scenePath.c_str());
			}
			else
				spdlog::warn("No scene path specified, not saving.");


		}
		ImGui::MenuItem("Save Scene As", "Ctrl+Shift+S");


		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Edit"))
	{
		ImGui::MenuItem("Undo", "Ctrl+Z");
		ImGui::MenuItem("Redo", "Ctrl+Y");
		ImGui::Separator();
		ImGui::MenuItem("Preferences");
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("View")) {
		ImGui::MenuItem("Inspector", nullptr, &showInspector);
		ImGui::MenuItem("Scene Explorer", nullptr, &showSceneExplorer);
		ImGui::MenuItem("Profiler", nullptr, &showProfiler);
		ImGui::MenuItem("Console", nullptr, &showConsole);
		ImGui::MenuItem("Skybox Settings", nullptr, &showSkyboxSettings);
		ImGui::MenuItem("View Cube", nullptr, &showViewCube);

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("GameObject")) {
		if (ImGui::MenuItem("Create Empty")) {
			engineService.create_entity();
		}

		ImGui::Separator();

		if (ImGui::BeginMenu("3D Object")) {
			if (ImGui::MenuItem("Cube")) {
				CreateCube();
			}
			if (ImGui::MenuItem("Physics Cube")) {
				CreatePhysicsCube();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Lights")) {
			if (ImGui::MenuItem("Directional Light")) {
				CreateLightEntity();
			}
			if (ImGui::MenuItem("Point Light")) {

			}
			if (ImGui::MenuItem("Spot Light")) {

			}

			ImGui::EndMenu();
		}
		if (ImGui::MenuItem("Create Camera")) {
			CreateCameraEntity();
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Help"))
	{
		if (ImGui::MenuItem("Dump Mono Reflection Tree"))
		{
			MonoReflectionRegistry::Instance().VisitAssemblies([](const AssemblyNode& assembly)
			{
				const std::string assemblyName = assembly.name.empty() ? std::string("<Unnamed>") : assembly.name;
				SPDLOG_INFO("[Reflection] Assembly: {}", assemblyName);

				for (const auto& [className, classNodePtr] : assembly.classes)
				{
					if (!classNodePtr)
					{
						continue;
					}

					const ClassNode& classNode = *classNodePtr;
					const MonoTypeDescriptor* classDescriptor = classNode.descriptor;
					const std::string classManaged = classDescriptor ? classDescriptor->managed_name : className;
					const std::string classCpp = classDescriptor && !classDescriptor->cpp_name.empty()
						? classDescriptor->cpp_name
						: std::string("<cpp unknown>");
					std::string classGuidStr;
					if (classDescriptor && classDescriptor->guid != rp::null_guid)
					{
						classGuidStr = classDescriptor->guid.to_hex();
					}
					std::string classExtra;
					if (!classGuidStr.empty())
					{
						classExtra += " | guid=";
						classExtra += classGuidStr;
					}
					SPDLOG_INFO("[Reflection]   Class: {} | managed=\"{}\" | cpp=\"{}\"{}", className, classManaged, classCpp, classExtra);

					for (const auto& [fieldName, fieldNodePtr] : classNode.fields)
					{
						if (!fieldNodePtr)
						{
							continue;
						}

						const FieldNode& fieldNode = *fieldNodePtr;
						if (!fieldNode.isPublic)
						{
							continue;
						}

						const MonoTypeDescriptor* fieldDescriptor = fieldNode.descriptor;
						const std::string fieldManaged = fieldDescriptor ? fieldDescriptor->managed_name : fieldNode.type;
						const std::string fieldCpp = fieldDescriptor && !fieldDescriptor->cpp_name.empty()
							? fieldDescriptor->cpp_name
							: std::string("<cpp unknown>");
						std::string fieldGuidStr;
						if (fieldDescriptor && fieldDescriptor->guid != rp::null_guid)
						{
							fieldGuidStr = fieldDescriptor->guid.to_hex();
						}
						std::string fieldExtra;
						if (fieldNode.isStatic)
						{
							fieldExtra += " [static]";
						}
						if (!fieldGuidStr.empty())
						{
							if (!fieldExtra.empty())
							{
								fieldExtra += " ";
							}
							fieldExtra += "| guid=";
							fieldExtra += fieldGuidStr;
						}
						SPDLOG_INFO("[Reflection]     Field: {} -> managed=\"{}\" | cpp=\"{}\"{}", fieldName, fieldManaged, fieldCpp, fieldExtra);
						if (fieldDescriptor && fieldDescriptor->elementDescriptor)
						{
							const MonoTypeDescriptor* element = fieldDescriptor->elementDescriptor;
							const std::string elementManaged = element->managed_name;
							const std::string elementCpp = !element->cpp_name.empty() ? element->cpp_name : std::string("<cpp unknown>");
							std::string elementGuidStr;
							if (element->guid != rp::null_guid)
							{
								elementGuidStr = element->guid.to_hex();
							}
							std::string elementExtra;
							if (!elementGuidStr.empty())
							{
								elementExtra += " | guid=";
								elementExtra += elementGuidStr;
							}
							SPDLOG_INFO("[Reflection]       Element -> managed=\"{}\" | cpp=\"{}\"{}", elementManaged, elementCpp, elementExtra);
						}
					}
				}
			});
		}

		ImGui::MenuItem("Documentation");
		if (ImGui::MenuItem("About"))
		{
			showAboutModal = true;
		}
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();

	Render_AboutUI();


}

void EditorMain::SetupUnityStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	// Main colors
	colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.196f, 0.196f, 0.196f, 1.00f); // #323232
	colors[ImGuiCol_ChildBg] = ImVec4(0.196f, 0.196f, 0.196f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.157f, 0.157f, 0.157f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.098f, 0.098f, 0.098f, 1.00f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.157f, 0.157f, 0.157f, 1.00f); // Input fields
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.235f, 0.235f, 0.235f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.275f, 0.275f, 0.275f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.157f, 0.157f, 0.157f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.196f, 0.196f, 0.196f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.157f, 0.157f, 0.157f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.157f, 0.157f, 0.157f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.157f, 0.157f, 0.157f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.275f, 0.275f, 0.275f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.314f, 0.314f, 0.314f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.353f, 0.353f, 0.353f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.337f, 0.612f, 0.839f, 1.00f); // Unity blue
	colors[ImGuiCol_SliderGrab] = ImVec4(0.337f, 0.612f, 0.839f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.400f, 0.690f, 0.890f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.235f, 0.235f, 0.235f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.275f, 0.275f, 0.275f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.314f, 0.314f, 0.314f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.235f, 0.235f, 0.235f, 1.00f); // Collapsing header
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.275f, 0.275f, 0.275f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.314f, 0.314f, 0.314f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.098f, 0.098f, 0.098f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.337f, 0.612f, 0.839f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.400f, 0.690f, 0.890f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.235f, 0.235f, 0.235f, 1.00f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.337f, 0.612f, 0.839f, 1.00f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.400f, 0.690f, 0.890f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.157f, 0.157f, 0.157f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.337f, 0.612f, 0.839f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.235f, 0.235f, 0.235f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.157f, 0.157f, 0.157f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.196f, 0.196f, 0.196f, 1.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.337f, 0.612f, 0.839f, 0.70f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.157f, 0.157f, 0.157f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.098f, 0.098f, 0.098f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.098f, 0.098f, 0.098f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.000f, 1.000f, 1.000f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.337f, 0.612f, 0.839f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.337f, 0.612f, 0.839f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	// Style adjustments
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.FramePadding = ImVec2(4.0f, 3.0f);
	style.ItemSpacing = ImVec2(8.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
	style.IndentSpacing = 21.0f;
	style.ScrollbarSize = 14.0f;
	style.GrabMinSize = 10.0f;

	style.WindowBorderSize = 1.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;
	style.TabBorderSize = 0.0f;

	style.WindowRounding = 0.0f;
	style.ChildRounding = 0.0f;
	style.FrameRounding = 3.0f;
	style.PopupRounding = 0.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabRounding = 3.0f;
	style.TabRounding = 4.0f;

	style.WindowTitleAlign = ImVec2(0.5f, 0.5f); // Center window titles
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f); // Center button text
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


void EditorMain::Setup_Dockspace(unsigned id)
{
	ImGuiID dockspace_id = id;

	if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
	{
		ImGui::DockBuilderRemoveNode(dockspace_id);                        // clear old layout
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // create root
		ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

		// Split and dock windows
		ImGuiID dock_main_id = dockspace_id;
		ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, nullptr, &dock_main_id);
		ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);

		ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_id_left);
		ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
		ImGui::DockBuilderDockWindow("Viewport", dock_main_id);

		ImGui::DockBuilderFinish(dockspace_id);
	}


}

void EditorMain::CreateObjectHelper() {
	if (ImGui::MenuItem("Create Empty")) {
		engineService.create_entity();
	}
	ImGui::Separator();

	if (ImGui::BeginMenu("3D Object")) {
		if (ImGui::MenuItem("Cube")) {
			CreateCube(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(1.0f), glm::vec3(0.5f, 0.5f, 1.0f));
		}
		if (ImGui::MenuItem("Plane")) {
			CreatePlaneEntity();
		}
		if (ImGui::MenuItem("Physics Cube")) {
			CreatePhysicsCube();
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Lights")) {
		if (ImGui::MenuItem("Directional Light")) {
			CreateLightEntity();
		}
		if (ImGui::MenuItem("Point Light")) {

		}
		if (ImGui::MenuItem("Spot Light")) {

		}

		ImGui::EndMenu();
	}
	if (ImGui::MenuItem("Create Camera")) {
		CreateCameraEntity();
	}
}

void EditorMain::Render_SceneExplorer()
{

	ImGui::Begin("Hierarchy", &showSceneExplorer);

	// === TOOLBAR (Unity-style) ===
	float toolbarHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 2;
	ImGui::BeginChild("HierarchyToolbar", ImVec2(0, toolbarHeight), true, ImGuiWindowFlags_NoScrollbar);
	{
		// Search bar
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60); // Leave space for + button
		ImGui::InputTextWithHint("##HierarchySearch", "Search...", m_HierarchySearchBuffer, sizeof(m_HierarchySearchBuffer));

		ImGui::SameLine();

		// Add object button (Unity-style '+' button)
		if (ImGui::Button("+", ImVec2(40, 0)))
		{
			ImGui::OpenPopup("CreateObjectPopup");
		}

		// Create object popup menu
		if (ImGui::BeginPopup("CreateObjectPopup"))
		{
			CreateObjectHelper();
			ImGui::EndPopup();
		}
	}
	ImGui::EndChild();

	// Scene tree
	/*const auto& entityHandles = engineService.GetEntitiesSnapshot();
	const auto& entityNames = engineService.GetEntityNamesSnapshot();*/
	//ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 16.0f);
	
	if (!engineService.m_cont) {
		ImGui::TextUnformatted("Scene data unavailable");
		ImGui::End();
		return;
	}

	std::unordered_map<rp::Guid, std::pair<std::shared_ptr<SceneGraphNode>, bool>> scnGraphs;
	{
		std::lock_guard guard{ engineService.m_cont->m_mtx };
		scnGraphs = engineService.m_cont->m_loaded_scenes_scenegraph_snapshot;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 16.0f);

	auto RenderSceneGraphNode{ [this] (const SceneGraphNode& node, rp::Guid scnguid){
		auto RenderNode{ [this, scnguid](const SceneGraphNode& node, auto&& fn) -> void {
			// Search filter - convert both to lowercase for case-insensitive search
			std::string searchFilter = m_HierarchySearchBuffer;
			std::string entityNameJump = node.m_entity_name;

			if (!searchFilter.empty()) {
				std::transform(searchFilter.begin(), searchFilter.end(), searchFilter.begin(), ::tolower);
				std::string lowerEntityName = entityNameJump;
				std::transform(lowerEntityName.begin(), lowerEntityName.end(), lowerEntityName.begin(), ::tolower);

				// Skip this node if it doesn't match the search filter
				if (lowerEntityName.find(searchFilter) == std::string::npos) {
					// Still recursively check children
					for (const auto& child : node.m_children) {
						fn(child, fn);
					}
					return;
				}
			}

			// Check if this entity is currently selected
			uint32_t entityUID = ecs::entity(node.m_entity_handle).get_uid();
			bool isSelected = (m_EntitiesIDSelection.find(entityUID) != m_EntitiesIDSelection.end());
			if (isSelected) {
				m_SelectedNodeID = node.m_entity_sid;
			}
			bool isDisabled{};
			int stylect{};

			// Check if this entity is a prefab instance
			bool isPrefabInstance = false;
			if (node.m_entity_handle) {
				ecs::entity entity(node.m_entity_handle);
				isPrefabInstance = entity.all<PrefabComponent>();
			}

			static const auto style_color_selected{ []() -> int {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.761f, 0.542f, 0.223f, 1.0f)); // Yellow text for selected
				ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.337f, 0.612f, 0.839f, 0.5f));
				ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 0.9f, 0.0f, 1.0f));
				return 3;
				} };
			static const auto style_color_inactive{ []() -> int {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.461f, 0.442f, 0.423f, 0.42f)); // Grayed text for inactive
				ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.15f, 0.123f, 0.10f));
				return 2;
				} };
			static const auto style_color_prefab{ []() -> int {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f)); // Blue text for prefab instances
				ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.6f, 0.3f));
				return 2;
				} };

			int(*current_style)() {nullptr};

			static const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

			// Highlight selected entity with different color
			if (isSelected && node.m_entity_handle) { //root node handle is null
				current_style = style_color_selected;
				stylect = current_style();
			}
			else if (!node.is_active) {
				current_style = style_color_inactive;
				stylect = current_style();
				isDisabled = true;
			}
			else if (isPrefabInstance) {
				current_style = style_color_prefab;
				stylect = current_style();
			}

		// Add prefab prefix for visual distinction (with nesting level)
		std::string displayName = node.m_entity_name;
		if (isPrefabInstance) {
			ecs::entity entity(node.m_entity_handle);
			const auto& prefabComp = entity.get<PrefabComponent>();

			// Show nesting level for nested prefabs
			if (prefabComp.IsNestedPrefabInstance()) {
				// Nested prefab: show level (e.g., [P:1], [P:2])
				displayName = "[P:" + std::to_string(prefabComp.m_NestingLevel) + "] " + displayName;
			} else {
				// Root prefab instance
				displayName = "[P] " + displayName;
			}
		}

		if (ImGui::TreeNodeEx((void*)(intptr_t)entityUID, flags, "%s", displayName.c_str())) {
			// Display entity info with selection highlighting
			std::string entityName = node.m_entity_name;

			//bool nodeOpen = ImGui::TreeNode(entityName.c_str());
			if (ImGui::IsItemClicked())
			{
				spdlog::info("DEBUG: Entity clicked - entityUID = {}, entityName = {}", entityUID, entityName);
				SelectEntity(entityUID);
				m_SelectedNodeID = node.m_entity_sid;
			}

			ImGui::PopStyleColor(stylect);
			stylect = 0;

			if (ImGui::BeginPopupContextItem())
			{
				if (node.m_entity_handle && ImGui::MenuItem("Duplicate"))
				{
					auto dupfn = [this, node]()
					{
						ecs::entity current{ node.m_entity_handle };
						auto  world = Engine::GetWorld();
						auto newEntity = world.add_entity();

						newEntity.add<VisibilityComponent>(current.get<VisibilityComponent>());
						newEntity.add<TransformComponent>(current.get<TransformComponent>());
						if (current.any<MeshRendererComponent>()) {
							newEntity.add<MeshRendererComponent>(current.get<MeshRendererComponent>());
						}
						if (current.any<MaterialOverridesComponent>()) {
							newEntity.add<MaterialOverridesComponent>(current.get<MaterialOverridesComponent>());
						}
						if (current.any<LightComponent>()) {
							newEntity.add<LightComponent>(current.get<LightComponent>());
						}

						if (current.any<CameraComponent>()) {
							newEntity.add<CameraComponent>(current.get<CameraComponent>());
						}

						if (current.any<ParticleComponent>())
							{
							newEntity.add<ParticleComponent>(current.get<ParticleComponent>());
						}	

						newEntity.name() = current.name() + "_Copy";

						
					};

					engineService.ExecuteOnEngineThread(dupfn);
				}
				if (node.m_entity_handle && ImGui::MenuItem("Delete"))
				{
					// Clear selection if deleting the currently selected entity
					if (isSelected) {
						ClearEntitySelection();
					}
					engineService.delete_entity(node.m_entity_handle);
				}
				if (ImGui::MenuItem(node.is_active ? "Disable" : "Enable"))
				{
					if(!node.m_entity_handle) {
						engineService.ExecuteOnEngineThread([node, scnguid]() {
							auto scnres{ Engine::GetSceneRegistry().GetScene(scnguid) };
							if (!scnres)
								return;
							if (node.is_active) {
								scnres.value().get().DisableScene();
							}
							else {
								scnres.value().get().EnableScene();
							}
							});
					}
					else {
						engineService.ExecuteOnEngineThread([node]() {auto e = ecs::entity(node.m_entity_handle);
						if (e.is_active()) {
							e.disable();
						}
						else {
							e.enable();
						}
							});
					}
				}
				if (node.m_entity_handle && ImGui::MenuItem("Create Child"))
				{
					engineService.create_child_entity(node.m_entity_handle);
				}

				// Prefab-specific menu items
				if (isPrefabInstance && node.m_entity_handle)
				{
					ImGui::Separator();
					ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Prefab");

					if (ImGui::MenuItem("Unpack Prefab"))
					{
						// Remove PrefabComponent to break connection with prefab
						engineService.ExecuteOnEngineThread([node]() {
							ecs::entity entity(node.m_entity_handle);
							if (entity.all<PrefabComponent>())
							{
								entity.remove<PrefabComponent>();
								spdlog::info("Unpacked prefab instance: {}", entity.name());
							}
						});
					}

					if (ImGui::MenuItem("Select Prefab Asset"))
					{
						// Navigate Asset Browser to prefab location
						if (node.m_entity_handle)
						{
							ecs::entity entity(node.m_entity_handle);
							if (entity.all<PrefabComponent>())
							{
								auto& prefabComp = entity.get<PrefabComponent>();
								std::string prefabPath = m_AssetManager->ResolveAssetName(prefabComp.m_PrefabGuid);

								if (!prefabPath.empty() && std::filesystem::exists(prefabPath))
								{
									// Get parent directory
									std::filesystem::path filePath(prefabPath);
									std::string parentDir = filePath.parent_path().string();

									// Navigate to parent directory in Asset Browser
									// Note: This assumes AssetManager has methods to navigate
									// If not in current path, navigate to it
									if (parentDir != m_AssetManager->GetCurrentPath())
									{
										// Navigate to root first, then to the target directory
										while (m_AssetManager->GetCurrentPath() != m_AssetManager->GetRootPath())
										{
											m_AssetManager->GoToParentDirectory();
										}

										// Navigate to target directory
										// This is a simplified approach - might need refinement
										m_AssetManager->GoToSubDirectory(parentDir);
									}

									spdlog::info("Navigated to prefab asset: {}", prefabPath);
								}
								else
								{
									spdlog::warn("Prefab asset not found: {}", prefabPath);
								}
							}
						}
					}

					if (ImGui::MenuItem("Revert to Prefab"))
					{
						engineService.ExecuteOnEngineThread([node]() {
							ecs::entity entity(node.m_entity_handle);
							auto world = Engine::GetWorld();
							PrefabSystem::RevertAllOverrides(world, entity);
						});
					}

					if (ImGui::MenuItem("Apply Overrides to Prefab"))
					{
						// Apply current instance state to prefab file
						engineService.ExecuteOnEngineThread([node, this]() {
							ecs::entity entity(node.m_entity_handle);
							auto world = Engine::GetWorld();

							if (entity.all<PrefabComponent>())
							{
								auto& prefabComp = entity.get<PrefabComponent>();
								std::string prefabPath = m_AssetManager->ResolveAssetName(prefabComp.m_PrefabGuid);

								if (!prefabPath.empty())
								{
									bool success = PrefabSystem::ApplyInstanceToPrefab(world, entity, prefabPath);
									if (success)
									{
										spdlog::info("Successfully applied overrides to prefab: {}", prefabPath);
										prefabComp.m_OverriddenProperties.clear();
									}
									else
									{
										spdlog::error("Failed to apply overrides to prefab: {}", prefabPath);
									}
								}
								else
								{
									spdlog::error("Failed to resolve prefab path from GUID");
								}
							}
						});
					}
				}
				else if (node.m_entity_handle && !isPrefabInstance)
				{
					ImGui::Separator();
					if (ImGui::MenuItem("Create Prefab from Entity"))
					{
						// Open file dialog to choose save location
						std::string prefabPath = m_AssetManager->GetCurrentPath() + "/" + node.m_entity_name + ".prefab";

						engineService.ExecuteOnEngineThread([node, prefabPath, this]() {
							ecs::entity entity(node.m_entity_handle);
							auto world = Engine::GetWorld();

							// Create prefab from entity
							std::string prefabName = node.m_entity_name + "_Prefab";
							rp::BasicIndexedGuid prefabGuid = PrefabSystem::CreatePrefabFromEntity(
								world, entity, prefabName, prefabPath
							);

							if (prefabGuid != rp::null_indexed_guid)
							{
								spdlog::info("Prefab created successfully: {} at {}", prefabName, prefabPath);

								// Attach PrefabComponent to the original entity to make it an instance
								entity.add<PrefabComponent>(prefabGuid);
							}
							else
							{
								spdlog::error("Failed to create prefab from entity: {}", node.m_entity_name);
							}
						});
					}
				}

				if (node.m_parent && node.m_parent->m_parent == nullptr)
				{
					if (ImGui::MenuItem("Create Parent", "Ctrl+G")) {
						(isSelected) ? engineService.create_parent_entity(m_EntitiesIDSelection) : engineService.create_parent_entity(node.m_entity_handle);
					}
					if (node.m_children.size() && ImGui::MenuItem("Orphan Children")) {
						engineService.orphan_children_entities(node.m_entity_handle);
					}
				}
				else if(node.m_entity_handle && ImGui::MenuItem("Clear Parent")) {
					engineService.clear_parent_entity(node.m_entity_handle);
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Rename")) { }
				ImGui::EndPopup();
			}
			
			if (current_style) {
				stylect = current_style();
			}
			
			//Draw children
			int imidx{};
			for (auto& child : node.m_children) {
				ImGui::PushID(imidx++);
				fn(child, fn);
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
		// Pop the selection highlight color if it was applied
		ImGui::PopStyleColor(stylect);
		stylect = 0;

		if (ImGui::BeginDragDropSource()) {
			// Payload can be any data type (e.g., pointer, ID, struct)
			ImGui::SetDragDropPayload("SCENE_HIERARCHY", &node, sizeof(SceneGraphNode));
			ImGui::Text("Dragging %s", node.m_entity_name.c_str());
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_HIERARCHY")) {
				const SceneGraphNode& dropped_node = *(const SceneGraphNode*)payload->Data;
				engineService.make_parent_entity(node.m_entity_handle, dropped_node.m_entity_handle);
			}
			ImGui::EndDragDropTarget();
		}

		} };
		RenderNode(node, RenderNode);
		} };

	if (scnGraphs.size()) {
		for (auto& [guid, kv] : scnGraphs) {
			auto& [scene, stale] = kv;
			RenderSceneGraphNode(*scene, guid);
		}
	}

	ImGui::PopStyleVar();

	// === DRAG-DROP: Accept prefab files from Asset Browser ===
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AssetDrop"))
		{
			// Get the dropped file path
			std::string droppedPath = static_cast<const char*>(payload->Data);

			// Check if it's a .prefab file
			std::filesystem::path filePath(droppedPath);
			if (filePath.extension() == ".prefab")
			{
				spdlog::info("Dropped prefab file into hierarchy: {}", droppedPath);

				// Instantiate the prefab
				engineService.ExecuteOnEngineThread([droppedPath, this]() {
					auto world = Engine::GetWorld();

					// Load and cache the prefab
					PrefabData prefabData = PrefabSystem::LoadAndCachePrefab(droppedPath);

					if (prefabData.IsValid())
					{
						// Instantiate at origin (or camera position in the future)
						ecs::entity instance = PrefabSystem::InstantiatePrefab(
							world,
							prefabData.guid,
							glm::vec3(0, 0, 0)
						);

						if (instance.get_uuid() != 0)
						{
							spdlog::info("Instantiated prefab '{}' in scene", prefabData.name);
						}
						else
						{
							spdlog::error("Failed to instantiate prefab: {}", prefabData.name);
						}
					}
					else
					{
						spdlog::error("Failed to load prefab from: {}", droppedPath);
					}
				});
			}
		}
		ImGui::EndDragDropTarget();
	}

	// Right-click in empty space to create new objects
	if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::BeginMenu("Create"))
		{
			if (ImGui::MenuItem("Empty GameObject"))
			{
				CreateDefaultEntity();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Cube"))
			{
				CreateCube(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(1.0f), glm::vec3(0.5f, 0.5f, 1.0f));
			}
			if (ImGui::MenuItem("Sphere")) {  }
			if (ImGui::MenuItem("Plane")) { CreatePlaneEntity(); }
			ImGui::Separator();
			if (ImGui::MenuItem("Camera")) { CreateCameraEntity(); }
			if (ImGui::MenuItem("Light")) { CreateLightEntity(); }
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}

	// Drag-and-drop target for prefab instantiation
	// Create invisible button to act as drop zone for remaining space
	ImVec2 dropZoneSize = ImGui::GetContentRegionAvail();
	if (dropZoneSize.y > 0)
	{
		ImGui::InvisibleButton("##HierarchyDropZone", dropZoneSize);

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AssetDrop"))
			{
				const char* droppedPath = static_cast<const char*>(payload->Data);
				std::string assetPath(droppedPath);
				std::filesystem::path filepath(assetPath);

				// Check if dropped asset is a prefab
				if (filepath.extension().string() == ".prefab")
				{
					engineService.ExecuteOnEngineThread([assetPath]() {
						auto world = Engine::GetWorld();
						PrefabData prefabData = PrefabSystem::LoadAndCachePrefab(assetPath.c_str());
						if (prefabData.IsValid()) {
							ecs::entity instantiated = PrefabSystem::InstantiatePrefab(world, prefabData.guid, glm::vec3(0.0f));
							if (instantiated) {
								spdlog::info("Prefab instantiated from drag-and-drop: {}", prefabData.name);
							} else {
								spdlog::error("Failed to instantiate prefab: {}", prefabData.name);
							}
						}
					});
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	ImGui::End();
	if((ImGui::IsKeyDown(ImGuiKey_LeftCtrl)|| ImGui::IsKeyDown(ImGuiKey_RightCtrl)) && ImGui::IsKeyPressed(ImGuiKey_G)) {
		engineService.create_parent_entity(m_EntitiesIDSelection);
	}
}


void EditorMain::Render_Profiler()
{
	ImGui::Begin("Profiler");

	auto info = engineService.GetEngineInfo();

	// Update FPS/Delta display at intervals for readability
	static double lastUpdateTime = 0.0;
	static double displayFPS = 0.0;
	static double displayDeltaTime = 0.0;
	static float updateInterval = 0.25f; // Update every 0.25 seconds

	double currentTime = glfwGetTime();
	if (currentTime - lastUpdateTime >= updateInterval) {
		displayFPS = info.m_FPS;
		displayDeltaTime = info.m_DeltaTime;
		lastUpdateTime = currentTime;
	}

	ImGui::Text("FPS: %.2f", displayFPS);
	ImGui::Text("Delta Time: %.3f ms", displayDeltaTime * 1000.0);
	ImGui::SliderFloat("Update Interval", &updateInterval, 0.1f, 2.0f, "%.2f s");
	ImGui::Separator();

	auto events = Profiler::instance().getEventCurrentFrame();
	auto last = Profiler::instance().Get_Last_Frame();



	double totalMs = 0.0;
	for (auto& kv : last.systemMs) {
		totalMs += kv.second;
	}

	for (auto& kv : last.systemMs) {
		double sysMs = kv.second;
		double pct = (totalMs > 0.0) ? (sysMs / totalMs) * 100.0 : 0.0;

		ImGui::Text("%s: %.2f ms (%.1f%%)", kv.first.c_str(), sysMs, pct);
	}

	ImGui::End();
}

void EditorMain::Render_Console()
{

	ImGui::Begin("Console");

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


	/*

	ImGui::Begin("Console", &showConsole);

	// Toolbar with filter buttons
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

	static bool showLogs = true;
	static bool showWarnings = true;
	static bool showErrors = true;

	// Clear button
	if (ImGui::Button("Clear"))
	{
		// Clear console logs
	}

	ImGui::SameLine();
	ImGui::Separator();
	ImGui::SameLine();

	// Filter toggles with icons
	ImGui::PushStyleColor(ImGuiCol_Button, showLogs ? ImVec4(0.235f, 0.235f, 0.235f, 1.0f) : ImVec4(0.157f, 0.157f, 0.157f, 1.0f));
	if (ImGui::Button("Info"))
		showLogs = !showLogs;
	ImGui::PopStyleColor();

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, showWarnings ? ImVec4(0.6f, 0.5f, 0.2f, 1.0f) : ImVec4(0.157f, 0.157f, 0.157f, 1.0f));
	if (ImGui::Button("Warning"))
		showWarnings = !showWarnings;
	ImGui::PopStyleColor();

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, showErrors ? ImVec4(0.7f, 0.2f, 0.2f, 1.0f) : ImVec4(0.157f, 0.157f, 0.157f, 1.0f));
	if (ImGui::Button("Error"))
		showErrors = !showErrors;
	ImGui::PopStyleColor();

	ImGui::PopStyleVar(2);

	ImGui::Separator();

	// Console output
	ImGui::BeginChild("ConsoleOutput", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	// Example log entries
	if (showLogs) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
		ImGui::TextUnformatted("[Info] Application started successfully");
		ImGui::PopStyleColor();
	}

	if (showWarnings) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.8f, 0.3f, 1.0f));
		ImGui::TextUnformatted("[Warning] Texture not found, using default");
		ImGui::PopStyleColor();
	}

	if (showErrors) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
		ImGui::TextUnformatted("[Error] Failed to load asset");
		ImGui::PopStyleColor();
	}

	ImGui::EndChild();
	*/
	ImGui::End();
}

void EditorMain::Render_SkyboxSettings()
{
	ImGui::Begin("Skybox Settings", &showSkyboxSettings);

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
	bool enabled = skyboxSettings.enabled;
	if (ImGui::Checkbox("Enable Skybox", &enabled)) {
		engineService.ExecuteOnEngineThread([enabled]() {
			auto& scene = Engine::GetSceneRegistry().GetActiveScene().value().get();
			scene.GetRenderSettings().skybox.enabled = enabled;
		});
	}

	ImGui::Separator();

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
			rp::BasicIndexedGuid indexed{skyboxSettings.faceTextures[i], 0};
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

	ImGui::Separator();

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

	ImGui::Separator();

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

	ImGui::Separator();
	ImGui::Text("Status:");
	if (skyboxSettings.cachedCubemapID != 0) {
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Cubemap loaded (GPU ID: %u)", skyboxSettings.cachedCubemapID);
	} else {
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No cubemap loaded");
	}

	// Show texture assignments
	int validTextures = 0;
	for (const auto& guid : skyboxSettings.faceTextures) {
		if (guid != rp::null_guid) validTextures++;
	}
	ImGui::Text("Face textures assigned: %d/6", validTextures);

	ImGui::End();
}

void EditorMain::Render_PhysicsDebugPanel()
{
	ImGui::Begin("Physics Debug", &showPhysicsDebug);

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
	auto& entityNames = engineService.m_cont->m_names_snapshot;

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

void EditorMain::Render_Game()
{
	ImGui::Begin("Game", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	// Get frame data from engine thread
	auto& frameData = engineService.GetFrameData();

	if (frameData.gameResolvedBuffer)
	{
		// Get texture from game framebuffer
		GLuint textureID = frameData.gameResolvedBuffer->GetColorAttachmentRendererID(0);

		// Get available viewport size
		ImVec2 viewportSize = ImGui::GetContentRegionAvail();

		// Display game camera view (flip Y-axis with ImVec2(0,1) to ImVec2(1,0))
		ImGui::Image(
			reinterpret_cast<void*>(static_cast<intptr_t>(textureID)),
			viewportSize,
			ImVec2(0, 1),  // UV coordinates flipped vertically
			ImVec2(1, 0)
		);
	}
	else
	{
		// No game camera or game buffer not initialized
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No active game camera");
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Add a Camera component with 'Is Active' enabled to see the game view");
	}

	ImGui::End();
}

void EditorMain::Render_CameraControls()
{
	ImGui::Begin("Camera Controls");

	if (m_EditorCamera) {
		// Camera mode selection
		ImGui::Text("Camera Mode:");
		const char* modes[] = { "Fly" };
		static int currentMode = 0;

		if (ImGui::Combo("Mode", &currentMode, modes, IM_ARRAYSIZE(modes))) {
			m_EditorCamera->SetMode(static_cast<EditorCamera::Mode>(currentMode));
		}

		ImGui::Separator();

		// Camera position info
		glm::vec3 pos = m_EditorCamera->GetPosition();
		ImGui::Text("Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);

		glm::vec3 rot = m_EditorCamera->GetRotation();
		ImGui::Text("Rotation: %.2f, %.2f, %.2f", rot.x, rot.y, rot.z);

		glm::vec3 target = m_EditorCamera->GetTarget();
		ImGui::Text("Target: %.2f, %.2f, %.2f", target.x, target.y, target.z);


		ImGui::Separator();

		// Camera settings
		float moveSpeed = m_EditorCamera->GetMoveSpeed();
		if (ImGui::SliderFloat("Move Speed", &moveSpeed, 0.1f, 50.0f)) {
			m_EditorCamera->SetMoveSpeed(moveSpeed);
		}

		float rotSpeed = m_EditorCamera->GetRotationSpeed();
		if (ImGui::SliderFloat("Rotation Speed", &rotSpeed, 0.1f, 1.0f)) {
			m_EditorCamera->SetRotationSpeed(rotSpeed);
		}

		ImGui::Separator();

		// Control instructions
		ImGui::Text("Controls:");
		ImGui::Text("  Right Click + Drag: Look around");
		ImGui::Text("  WASD: Move horizontally");
		ImGui::Text("  Q/E: Move up/down");
		ImGui::Text("  Shift: Speed boost");
		ImGui::Text("  Scroll: Adjust move speed");

		ImGui::Separator();

		// Reset button
		if (ImGui::Button("Reset Camera")) {
			m_EditorCamera->Reset();
		}

		// Focus on origin button
		ImGui::SameLine();
		if (ImGui::Button("Focus Origin")) {
			m_EditorCamera->FocusOn(glm::vec3(0.0f), 10.0f);
		}
	}
	else {
		ImGui::Text("Camera not initialized");
	}

	ImGui::End();
}

void EditorMain::Render_AssetBrowser()
{
	// Padding Variables: Asset Grid/List
	static float thumbnailSize = 72.0f;
	static float padding = thumbnailSize * 0.1f; // Unity-like: padding is 10% of thumbnail size
	static bool initialized = false;
	if (!initialized) {
		padding = thumbnailSize * 0.1f;
		initialized = true;
	}

	ImGui::Begin("Asset Browser", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	// Calculate heights for all sections to ensure everything fits
	float toolbarHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 2;
	float statusBarHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 2;
	float itemSpacing = ImGui::GetStyle().ItemSpacing.y;

	// === TOOLBAR ===
	ImGui::BeginChild("AssetToolbar", ImVec2(0, toolbarHeight), true, ImGuiWindowFlags_NoScrollbar);
	{
		// Search bar
		ImGui::SetNextItemWidth(200.0f);
		ImGui::InputTextWithHint("##AssetSearch", "Search assets...", m_AssetSearchBuffer, sizeof(m_AssetSearchBuffer));

		ImGui::SameLine();

		// View mode toggle
		ImGui::Text("|");
		ImGui::SameLine();
		if (ImGui::Button(m_AssetViewMode == AssetViewMode::Grid ? "Grid View" : "List View"))
		{
			m_AssetViewMode = (m_AssetViewMode == AssetViewMode::Grid) ? AssetViewMode::List : AssetViewMode::Grid;
		}

		ImGui::SameLine();

		// Create menu
		if (ImGui::Button("Create"))
		{
			ImGui::OpenPopup("CreateAssetPopup");
		}

		if (ImGui::BeginPopup("CreateAssetPopup"))
		{
			if (ImGui::MenuItem("Material"))
			{
				m_ShowCreateMaterialDialog = true;
			}
			ImGui::EndPopup();
		}
	}
	ImGui::EndChild();

	// === MAIN CONTENT AREA (Split panel) ===
	static float folderTreeWidth = 200.0f;

	// Leave space for status bar and spacing
	ImGui::BeginChild("AssetContent", ImVec2(0, -(statusBarHeight + itemSpacing)));
	{
		// LEFT PANEL: Folder Tree
		ImGui::BeginChild("FolderTree", ImVec2(m_FolderTreeWidth, 0), true);
		{
			ImGui::Text("Folders");
			ImGui::Separator();

			// Recursive folder tree helper
			std::function<void(const std::string&)> RenderFolderTree = [&](const std::string& path)
			{
				std::filesystem::path fsPath(path);
				std::string folderName = (path == m_AssetManager->GetRootPath()) ? "Assets" : fsPath.filename().string();

				bool isCurrentPath = (path == m_AssetManager->GetCurrentPath());

				// Get subdirectories for this path
				std::vector<std::string> subdirs;
				try {
					for (const auto& entry : std::filesystem::directory_iterator(path))
					{
						if (entry.is_directory())
						{
							subdirs.push_back(entry.path().string());
						}
					}
				} catch (...) {}

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
				if (subdirs.empty())
					flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				if (isCurrentPath)
					flags |= ImGuiTreeNodeFlags_Selected;

				bool nodeOpen = ImGui::TreeNodeEx(folderName.c_str(), flags);

				// Click to navigate
				if (ImGui::IsItemClicked())
				{
					m_AssetManager->SetCurrentPath(path);
					m_SelectedAssetPath = "";
				}

				if (nodeOpen && !subdirs.empty())
				{
					for (const auto& subdir : subdirs)
					{
						RenderFolderTree(subdir);
					}
					ImGui::TreePop();
				}
			};

			RenderFolderTree(m_AssetManager->GetRootPath());
		}
		ImGui::EndChild();

		// Splitter
		ImGui::SameLine();
		ImGui::Button("##splitter", ImVec2(4.0f, -1));
		if (ImGui::IsItemActive())
		{
			m_FolderTreeWidth += ImGui::GetIO().MouseDelta.x;
			m_FolderTreeWidth = glm::clamp(m_FolderTreeWidth, 100.0f, 400.0f);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
		}
		ImGui::SameLine();

		// RIGHT PANEL: Asset Grid/List
		ImGui::BeginChild("AssetGrid", ImVec2(0, 0), true);
		{

			// === BREADCRUMB NAVIGATION ===
			std::filesystem::path currentPath = m_AssetManager->GetCurrentPath();
			std::filesystem::path rootPath = m_AssetManager->GetRootPath();

			// Build path segments
			std::vector<std::filesystem::path> pathSegments;
			std::filesystem::path tempPath = currentPath;

			while (tempPath != rootPath && !tempPath.empty())
			{
				pathSegments.insert(pathSegments.begin(), tempPath);
				tempPath = tempPath.parent_path();
			}
			pathSegments.insert(pathSegments.begin(), rootPath);

			// Render clickable breadcrumbs
			for (size_t i = 0; i < pathSegments.size(); ++i)
			{
				if (i > 0)
				{
					ImGui::SameLine();
					ImGui::TextDisabled(">");
					ImGui::SameLine();
				}

				std::string segmentName = (i == 0) ? "Assets" : pathSegments[i].filename().string();

				if (ImGui::SmallButton(segmentName.c_str()))
				{
					// Navigate to this path
					m_AssetManager->SetCurrentPath(pathSegments[i].string());
				}
			}

			std::vector<std::string> subdirs = m_AssetManager->GetSubDirectories();
			auto files = m_AssetManager->GetFiles(m_AssetManager->GetCurrentPath());

			// Filter by search
			std::string searchFilter = m_AssetSearchBuffer;
			std::transform(searchFilter.begin(), searchFilter.end(), searchFilter.begin(), ::tolower);

			bool mClickDouble = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

			if (m_AssetViewMode == AssetViewMode::Grid)
			{
				// === GRID VIEW ===
				float cellSize = thumbnailSize + padding;
				float panelWidth = ImGui::GetContentRegionAvail().x;
				int columns = static_cast<int>(panelWidth / cellSize);
				if (columns <= 0) columns = 1;

				ImGui::Columns(columns, 0, false);

				// Render folders first
				for (const std::string& subd : subdirs)
				{
					std::filesystem::path subpath{ subd };
					std::string folderName = subpath.filename().string();

					// Search filter
					if (!searchFilter.empty())
					{
						std::string lowerName = folderName;
						std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
						if (lowerName.find(searchFilter) == std::string::npos)
							continue;
					}

					ImGui::PushID(folderName.c_str());

					bool isSelected = (m_SelectedAssetPath == subd);

					// Draw selection border if selected
					if (isSelected)
					{
						ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.7f, 1.0f, 1.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
					}

					// Use icon if available, otherwise fallback to button with text
					if (m_AssetIcons.folderIcon != 0)
					{
						// Draw icon as button
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0)); // Transparent button
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.3f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));

						if (ImGui::ImageButton(folderName.c_str(), (ImTextureID)(uintptr_t)m_AssetIcons.folderIcon,
							ImVec2(thumbnailSize, thumbnailSize), ImVec2(0, 0), ImVec2(1, 1)))
						{
							m_SelectedAssetPath = subd;
						}

						ImGui::PopStyleColor(3);
					}
					else
					{
						// Fallback: colored button with text
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.7f, 0.3f, 0.4f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.8f, 0.4f, 0.6f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.9f, 0.5f, 0.8f));

						if (ImGui::Button("[ FOLDER ]", { thumbnailSize, thumbnailSize }))
						{
							m_SelectedAssetPath = subd;
						}

						ImGui::PopStyleColor(3);
					}

					if (isSelected)
					{
						ImGui::PopStyleVar();
						ImGui::PopStyleColor();
					}

					// Double-click to open
					if (ImGui::IsItemHovered() && mClickDouble)
					{
						m_AssetManager->GoToSubDirectory(subd);
						m_SelectedAssetPath = "";
					}

					// Right-click context menu
					if (ImGui::BeginPopupContextItem())
					{
						if (ImGui::MenuItem("Import All"))
						{
							auto biguids{ m_AssetManager->ImportAssetDirectory(subd) };
							engineService.ExecuteOnEngineThread([biguids] {
								std::for_each(biguids.begin(), biguids.end(), [](rp::BasicIndexedGuid biguid) {
									ResourceRegistry::Instance().Unload(biguid);
								});
							});
						}
						ImGui::EndPopup();
					}

					// Clip filename if too long instead of wrapping
					ImGui::Text("%s", folderName.c_str());
					ImGui::NextColumn();
					ImGui::PopID();
				}

				// Render files
				static bool ShowImportSettingsMenu = false;
				for (auto it = files.first; it != files.second; ++it)
				{
					std::filesystem::path filepath{ it->second };
					std::string filename = filepath.filename().string();

					if (filename.empty()) continue;

					// Search filter
					if (!searchFilter.empty())
					{
						std::string lowerName = filename;
						std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
						if (lowerName.find(searchFilter) == std::string::npos)
							continue;
					}

					ImGui::PushID(filename.c_str());

					// Skip descriptor files (.Desc extension)
					std::string ext = filepath.extension().string();

					bool isSelected = (m_SelectedAssetPath == it->second);

					// Draw selection border if selected
					if (isSelected)
					{
						ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.7f, 1.0f, 1.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
					}

					// Get appropriate icon for file type
					unsigned int fileIcon = GetIconForFile(ext);

					// Use icon if available, otherwise fallback to button with text
					if (fileIcon != 0)
					{
						// Draw icon as button
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0)); // Transparent button
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.3f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));

						if (ImGui::ImageButton(filename.c_str(), (ImTextureID)(uintptr_t)fileIcon,
							ImVec2(thumbnailSize, thumbnailSize), ImVec2(0, 0), ImVec2(1, 1)))
						{
							m_SelectedAssetPath = it->second;
						}

						ImGui::PopStyleColor(3);
					}
					else
					{
						// Fallback: button with file extension as text
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 0.4f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.6f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.6f, 0.8f));

						std::string buttonLabel = ext.empty() ? "[ FILE ]" : ext;
						if (ImGui::Button(buttonLabel.c_str(), { thumbnailSize, thumbnailSize }))
						{
							m_SelectedAssetPath = it->second;
						}

						ImGui::PopStyleColor(3);
					}

					if (isSelected)
					{
						ImGui::PopStyleVar();
						ImGui::PopStyleColor();
					}

					// Right-click context menu
					if (ImGui::BeginPopupContextItem())
					{
						if (ImGui::MenuItem("Import Asset"))
						{
							rp::BasicIndexedGuid biguid{ m_AssetManager->ImportAsset(it->second) };
							engineService.ExecuteOnEngineThread([biguid] {
								ResourceRegistry::Instance().Unload(biguid);
							});
						}
						if (ImGui::MenuItem("Import Settings"))
						{
							m_AssetManager->LoadImportSettings(it->second);
							ShowImportSettingsMenu = true;
						}
						ImGui::EndPopup();
					}

					if (ShowImportSettingsMenu)
					{
						ImGui::OpenPopup("DescriptorInspector");
						ShowImportSettingsMenu = false;
					}
					Render_ImporterSettings();

					// Drag and drop
					if (ImGui::BeginDragDropSource())
					{
						std::string itemPath = filepath.string();
						char AssetPayload[] = "AssetDrop";
						ImGui::SetDragDropPayload(AssetPayload, itemPath.c_str(), strlen(itemPath.c_str()) + 1);
						ImGui::EndDragDropSource();
					}

					// Clip filename if too long instead of wrapping
					ImGui::Text("%s", filename.c_str());
					ImGui::NextColumn();
					ImGui::PopID();
				}

				ImGui::Columns(1);
			}
			else
			{
				// === LIST VIEW ===
				ImGui::Columns(3);
				ImGui::Separator();
				ImGui::Text("Name"); ImGui::NextColumn();
				ImGui::Text("Type"); ImGui::NextColumn();
				ImGui::Text("Path"); ImGui::NextColumn();
				ImGui::Separator();

				// Folders
				for (const std::string& subd : subdirs)
				{
					std::filesystem::path subpath{ subd };
					std::string folderName = subpath.filename().string();

					if (!searchFilter.empty())
					{
						std::string lowerName = folderName;
						std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
						if (lowerName.find(searchFilter) == std::string::npos)
							continue;
					}

					ImGui::PushID(folderName.c_str());

					bool isSelected = (m_SelectedAssetPath == subd);
					if (ImGui::Selectable(folderName.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
					{
						m_SelectedAssetPath = subd;
					}

					if (ImGui::IsItemHovered() && mClickDouble)
					{
						m_AssetManager->GoToSubDirectory(subd);
						m_SelectedAssetPath = "";
					}

					if (ImGui::BeginPopupContextItem())
					{
						if (ImGui::MenuItem("Import All"))
						{
							auto biguids{ m_AssetManager->ImportAssetDirectory(subd) };
							engineService.ExecuteOnEngineThread([biguids] {
								std::for_each(biguids.begin(), biguids.end(), [](rp::BasicIndexedGuid biguid) {
									ResourceRegistry::Instance().Unload(biguid);
								});
							});
						}
						ImGui::EndPopup();
					}

					ImGui::NextColumn();
					ImGui::Text("Folder"); ImGui::NextColumn();
					ImGui::TextDisabled("%s", subpath.parent_path().string().c_str()); ImGui::NextColumn();

					ImGui::PopID();
				}

				// Files
				static bool ShowImportSettingsMenu = false;
				for (auto it = files.first; it != files.second; ++it)
				{
					std::filesystem::path filepath{ it->second };
					std::string filename = filepath.filename().string();

					if (filename.empty()) continue;

					if (!searchFilter.empty())
					{
						std::string lowerName = filename;
						std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
						if (lowerName.find(searchFilter) == std::string::npos)
							continue;
					}

					ImGui::PushID(filename.c_str());

					bool isSelected = (m_SelectedAssetPath == it->second);
					if (ImGui::Selectable(filename.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
					{
						m_SelectedAssetPath = it->second;
					}

					if (ImGui::BeginPopupContextItem())
					{
						if (ImGui::MenuItem("Import Asset"))
						{
							rp::BasicIndexedGuid biguid{ m_AssetManager->ImportAsset(it->second) };
							engineService.ExecuteOnEngineThread([biguid] {
								ResourceRegistry::Instance().Unload(biguid);
							});
						}
						if (ImGui::MenuItem("Import Settings"))
						{
							m_AssetManager->LoadImportSettings(it->second);
							ShowImportSettingsMenu = true;
						}
						ImGui::EndPopup();
					}

					if (ShowImportSettingsMenu)
					{
						ImGui::OpenPopup("DescriptorInspector");
						ShowImportSettingsMenu = false;
					}
					Render_ImporterSettings();

					if (ImGui::BeginDragDropSource())
					{
						std::string itemPath = filepath.string();
						char AssetPayload[] = "AssetDrop";
						ImGui::SetDragDropPayload(AssetPayload, itemPath.c_str(), strlen(itemPath.c_str()) + 1);
						ImGui::EndDragDropSource();
					}

					ImGui::NextColumn();
					std::string ext = filepath.extension().string();
					ImGui::Text("%s", ext.c_str()); ImGui::NextColumn();
					ImGui::TextDisabled("%s", filepath.parent_path().string().c_str()); ImGui::NextColumn();

					ImGui::PopID();
				}

				ImGui::Columns(1);
			}

			// Context menu for empty space
			if (ImGui::BeginPopupContextWindow("AssetBrowserContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
			{
				if (ImGui::BeginMenu("Create"))
				{
					if (ImGui::MenuItem("Material"))
					{
						m_ShowCreateMaterialDialog = true;
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();

	// === STATUS BAR ===
	ImGui::BeginChild("StatusBar", ImVec2(0, statusBarHeight), true, ImGuiWindowFlags_NoScrollbar);
	{
		// Left side: Item count or selection info
		if (!m_SelectedAssetPath.empty())
		{
			std::filesystem::path selectedPath(m_SelectedAssetPath);
			ImGui::Text("Selected: %s", selectedPath.filename().string().c_str());
		}
		else
		{
			auto files = m_AssetManager->GetFiles(m_AssetManager->GetCurrentPath());
			int fileCount = static_cast<int>(std::distance(files.first, files.second));
			int folderCount = static_cast<int>(m_AssetManager->GetSubDirectories().size());
			ImGui::Text("%d items (%d folders, %d files)", fileCount + folderCount, folderCount, fileCount);
		}

		// Right side: Thumbnail size slider (only in Grid view)
		if (m_AssetViewMode == AssetViewMode::Grid)
		{
			ImGui::SameLine(ImGui::GetWindowWidth() - 250); // Position on right side
			ImGui::SetNextItemWidth(200);
			if (ImGui::SliderFloat("##ThumbnailSize", &thumbnailSize, 32.0f, 256.0f, "Size: %.0f"))
			{
				// Auto-adjust padding based on thumbnail size (Unity-like behavior)
				// Padding is approximately 10% of thumbnail size
				padding = thumbnailSize * 0.1f;
			}
		}
	}
	ImGui::EndChild();

	// === MATERIAL CREATION DIALOG ===
	if (m_ShowCreateMaterialDialog)
	{
		ImGui::OpenPopup("Create Material");
		m_ShowCreateMaterialDialog = false;
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Create Material", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Enter material name:");
		ImGui::Separator();

		ImGui::InputText("##MaterialName", m_NewMaterialNameBuffer, sizeof(m_NewMaterialNameBuffer));

		ImGui::Separator();

		if (ImGui::Button("Create", ImVec2(120, 0)))
		{
			if (strlen(m_NewMaterialNameBuffer) > 0)
			{
				m_AssetManager->CreateMaterialDescriptor(std::string(m_NewMaterialNameBuffer));
				ImGui::CloseCurrentPopup();
				strcpy(m_NewMaterialNameBuffer, "NewMaterial");
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			strcpy(m_NewMaterialNameBuffer, "NewMaterial");
		}

		ImGui::EndPopup();
	}

	ImGui::End();

	// === RESOURCES WINDOW (Imported Assets) ===
	ImGui::Begin("Resources");
	{
		static float resPadding = 8.0f;
		static float resThumbnailSize = 72.0f;
		float resCellSize = resThumbnailSize + resPadding;
		float resPanelWidth = ImGui::GetContentRegionAvail().x;
		int resColumns = static_cast<int>(resPanelWidth / resCellSize);
		if (resColumns <= 0) resColumns = 1;

		ImGui::Columns(resColumns, 0, false);

		for (auto [assetname, guid] : m_AssetManager->m_AssetNameGuid)
		{
			ImGui::PushID(assetname.c_str());
			ImGui::Button(assetname.c_str(), { resThumbnailSize, resThumbnailSize });
			ImGui::TextWrapped("%s", assetname.c_str());
			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::Columns(1);
	}
	ImGui::End();
}

void EditorMain::Render_AssetBrowser_Old()
{
	ImGui::Begin("Files");
	ImGui::Text(m_AssetManager->GetCurrentPath().c_str());
	if (m_AssetManager->GetCurrentPath() != m_AssetManager->GetRootPath())
	{
		if (ImGui::Button("Back"))
		{
			m_AssetManager->GoToParentDirectory();
		}
	}
	std::vector<std::string> subdir = m_AssetManager->GetSubDirectories();

	// Variables for spacing
	static float padding = 5.0f;
	static float thumbnailSize = 72.0f;
	float cellSize = thumbnailSize + padding;

	bool mClickDouble = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
	float panelWidth = ImGui::GetContentRegionAvail().x;
	int columns = static_cast<int>(panelWidth / cellSize);

	if (columns <= 0)
	{
		columns = 1;
	}
	ImGui::Columns(columns, 0, false);

	for (std::string const& subd : subdir)
	{
		std::filesystem::path subpath{ subd };
		std::string FolderName = subpath.filename().string();
		ImGui::PushID(FolderName.c_str());
		ImGui::Button("Folder", { thumbnailSize, thumbnailSize }); // Creates a button for the folders
		if (ImGui::IsItemHovered() && mClickDouble) // Double Click into a folder 
		{
			m_AssetManager->GoToSubDirectory(subd);
		}
		if (ImGui::BeginPopupContextItem()) // if you right click on an asset
		{
			if (ImGui::MenuItem("Import All")) // popup asking to import asset
			{
				auto biguids{ m_AssetManager->ImportAssetDirectory(subd) };
				engineService.ExecuteOnEngineThread([biguids] {
					std::for_each(biguids.begin(), biguids.end(), [](rp::BasicIndexedGuid biguid) {
						ResourceRegistry::Instance().Unload(biguid);
						});
					});
			}
			ImGui::EndPopup();
		}
		ImGui::TextWrapped(FolderName.c_str()); // Name of the asset

		ImGui::NextColumn();
		ImGui::PopID();
	}


	auto files = m_AssetManager->GetFiles(m_AssetManager->GetCurrentPath());
	static bool ShowImportSettingsMenu = false;

	for (auto it = files.first; it != files.second; ++it) {
		std::filesystem::path filepath{ it->second };
		std::string filename = filepath.filename().string();

		if (filename.empty()) continue;

		// Check if this is a prefab file
		bool isPrefabFile = (filepath.extension().string() == ".prefab");

		ImGui::PushID(filename.c_str());

		// Style prefab files differently
		if (isPrefabFile) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 0.6f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.7f, 0.8f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));
		}

		ImGui::Button(filename.c_str(), { thumbnailSize, thumbnailSize }); // Creates a button for the folders

		if (isPrefabFile) {
			ImGui::PopStyleColor(3);
		}

		if (ImGui::BeginPopupContextItem()) // if you right click on an asset
		{
			if (isPrefabFile) {
				// Prefab-specific menu items
				if (ImGui::MenuItem("Instantiate Prefab")) {
					std::string prefabPath = it->second;
					engineService.ExecuteOnEngineThread([prefabPath]() {
						auto world = Engine::GetWorld();
						PrefabData prefabData = PrefabSystem::LoadAndCachePrefab(prefabPath.c_str());
						if (prefabData.IsValid()) {
							ecs::entity instance = PrefabSystem::InstantiatePrefab(world, prefabData.guid, glm::vec3(0.0f));
							if (instance) {
								spdlog::info("Successfully instantiated prefab: {}", prefabData.name);
							} else {
								spdlog::error("Failed to instantiate prefab: {}", prefabData.name);
							}
						}
					});
				}
				ImGui::Separator();
			}

			if (ImGui::MenuItem("Import Asset")) // popup asking to import asset
			{
				rp::BasicIndexedGuid biguid{ m_AssetManager->ImportAsset(it->second) };
				engineService.ExecuteOnEngineThread([biguid] {
					ResourceRegistry::Instance().Unload(biguid);
					});
			}
			if (ImGui::MenuItem("Import Settings")) // popup asking to import asset
			{
				m_AssetManager->LoadImportSettings(it->second);
				ShowImportSettingsMenu = true;
			}
			ImGui::EndPopup();
		}

		if (ShowImportSettingsMenu) {
			ImGui::OpenPopup("DescriptorInspector");
			ShowImportSettingsMenu = false;
		}
		Render_ImporterSettings();

		if (ImGui::BeginDragDropSource()) // If we start dragging
		{
			std::string itemPath = filepath.string();
			char AssetPayload[] = "AssetDrop"; // Names any payload that starts from the asset browser to "AssetDrop"
			ImGui::SetDragDropPayload(AssetPayload, itemPath.c_str(), strlen(itemPath.c_str()) + 1); // Actually load the payload with the data(the string name of the path)
			ImGui::EndDragDropSource(); // Wraps up the payload package
		}

		ImGui::TextWrapped(filename.c_str()); // Name of the asset
		ImGui::NextColumn();
		ImGui::PopID();
	}

	// Context menu for empty space (right-click to create new assets)
	if (ImGui::BeginPopupContextWindow("AssetBrowserContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::BeginMenu("Create"))
		{
			if (ImGui::MenuItem("Material"))
			{
				m_ShowCreateMaterialDialog = true;
			}
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}

	// Material creation dialog
	if (m_ShowCreateMaterialDialog)
	{
		ImGui::OpenPopup("Create Material");
		m_ShowCreateMaterialDialog = false;
	}

	// Center the modal
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Create Material", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Enter material name:");
		ImGui::Separator();

		ImGui::InputText("##MaterialName", m_NewMaterialNameBuffer, sizeof(m_NewMaterialNameBuffer));

		ImGui::Separator();

		if (ImGui::Button("Create", ImVec2(120, 0)))
		{
			// Validate name is not empty
			if (strlen(m_NewMaterialNameBuffer) > 0)
			{
				m_AssetManager->CreateMaterialDescriptor(std::string(m_NewMaterialNameBuffer));
				ImGui::CloseCurrentPopup();
				// Reset buffer for next time
				strcpy(m_NewMaterialNameBuffer, "NewMaterial");
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			// Reset buffer
			strcpy(m_NewMaterialNameBuffer, "NewMaterial");
		}

		ImGui::EndPopup();
	}

	ImGui::Columns(1);

	ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 32.0f, 256.0f); // Slider for thumbnail size
	ImGui::SliderFloat("Padding Size", &padding, 0.0f, 24.0f); // Slider for how much padding between columns
	ImGui::End();

	ImGui::Begin("Resources");
	ImGui::Columns(columns, 0, false);
	//this map is single threaded
	for (auto [assetname, guid] : m_AssetManager->m_AssetNameGuid) {
		ImGui::PushID(assetname.c_str());
		ImGui::Button(assetname.c_str(), { thumbnailSize, thumbnailSize });
		ImGui::TextWrapped(assetname.c_str()); // Name of the asset
		ImGui::NextColumn();
		ImGui::PopID();
	}
	ImGui::Columns(1);
	ImGui::End();
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

void EditorMain::LoadAssetIcons()
{
	// Try to load icon textures from assets/icons/ directory
	// If they don't exist, the texture ID will remain 0 (fallback to text)
	std::string iconPath = "assets/icons/";

	// Attempt to load each icon (will fail gracefully if files don't exist)
	try {
		m_AssetIcons.folderIcon = TextureLoader::TextureFromFile("folder.png", iconPath.c_str(), false);
	} catch (...) {
		spdlog::warn("Failed to load folder icon");
	}

	try {
		m_AssetIcons.fileIcon = TextureLoader::TextureFromFile("file.png", iconPath.c_str(), false);
	} catch (...) {
		spdlog::warn("Failed to load file icon");
	}

	try {
		m_AssetIcons.imageIcon = TextureLoader::TextureFromFile("image.png", iconPath.c_str(), false);
	} catch (...) {
		spdlog::warn("Failed to load image icon");
	}

	try {
		m_AssetIcons.modelIcon = TextureLoader::TextureFromFile("model.png", iconPath.c_str(), false);
	} catch (...) {
		spdlog::warn("Failed to load model icon");
	}

	try {
		m_AssetIcons.audioIcon = TextureLoader::TextureFromFile("audio.png", iconPath.c_str(), false);
	} catch (...) {
		spdlog::warn("Failed to load audio icon");
	}

	try {
		m_AssetIcons.scriptIcon = TextureLoader::TextureFromFile("script.png", iconPath.c_str(), false);
	} catch (...) {
		spdlog::warn("Failed to load script icon");
	}

	try {
		m_AssetIcons.materialIcon = TextureLoader::TextureFromFile("material.png", iconPath.c_str(), false);
	} catch (...) {
		spdlog::warn("Failed to load material icon");
	}

	try {
		m_AssetIcons.shaderIcon = TextureLoader::TextureFromFile("shader.png", iconPath.c_str(), false);
	} catch (...) {
		spdlog::warn("Failed to load shader icon");
	}

	spdlog::info("Asset icons loaded");
}

unsigned int EditorMain::GetIconForFile(const std::string& extension)
{
	// Convert to lowercase for case-insensitive comparison
	std::string ext = extension;
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	// Image files
	if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
		ext == ".tga" || ext == ".dds" || ext == ".hdr") {
		return m_AssetIcons.imageIcon;
	}
	// Model files
	else if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".dae") {
		return m_AssetIcons.modelIcon;
	}
	// Audio files
	else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") {
		return m_AssetIcons.audioIcon;
	}
	// Script files
	else if (ext == ".cs" || ext == ".lua" || ext == ".py" || ext == ".js") {
		return m_AssetIcons.scriptIcon;
	}
	// Material files
	else if (ext == ".mat" || ext == ".material") {
		return m_AssetIcons.materialIcon;
	}
	// Shader files
	else if (ext == ".shader" || ext == ".vert" || ext == ".frag" || ext == ".glsl" ||
			 ext == ".vs" || ext == ".fs" || ext == ".comp") {
		return m_AssetIcons.shaderIcon;
	}
	// Prefab files (Unity-style prefab assets)
	else if (ext == ".prefab") {
		// Use script icon for now (could add custom prefab icon in the future)
		// Prefabs are data/template files, similar to scripts in nature
		return m_AssetIcons.scriptIcon != 0 ? m_AssetIcons.scriptIcon : m_AssetIcons.fileIcon;
	}
	// Default file icon
	return m_AssetIcons.fileIcon;
}

void EditorMain::Render_Scene()
{
	// Get delta time for camera updates
	float deltaTime = static_cast<float>(engineService.GetDeltaTime());

	ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	 
	// Get viewport size for aspect ratio
	ImVec2 viewportSize = ImGui::GetContentRegionAvail();
	if (viewportSize.x > 0 && viewportSize.y > 0) {
		m_ViewportWidth = viewportSize.x;
		m_ViewportHeight = viewportSize.y;

		// Update aspect ratio if viewport size changed
		if (m_EditorCamera) {
			m_EditorCamera->SetAspectRatio(m_ViewportWidth / m_ViewportHeight);
		}

		// Debug viewport size
		static float lastWidth = 0, lastHeight = 0;
		if (std::abs(lastWidth - viewportSize.x) > 1.0f || std::abs(lastHeight - viewportSize.y) > 1.0f) {
			spdlog::info("Editor: Viewport size changed to ({:.0f}x{:.0f})", viewportSize.x, viewportSize.y);
			lastWidth = viewportSize.x;
			lastHeight = viewportSize.y;
		}
	}

	// Check if editor framebuffer is available before trying to access it
	auto& frameData = engineService.GetFrameData();
	if (frameData.editorResolvedBuffer && frameData.editorResolvedBuffer->GetFBOHandle() != 0) {
		// Store viewport position for picking calculations
		ImVec2 viewportPos = ImGui::GetCursorScreenPos();
		// Push viewport rect to camera system so ScreenPointToRay/World use the correct offset
		engineService.ExecuteOnEngineThread([viewportPos, viewportSize]() {
			CameraSystem::SetViewportOffset(glm::vec2{ viewportPos.x, viewportPos.y });
			CameraSystem::SetViewportSize(glm::vec2{ viewportSize.x, viewportSize.y });
		});

		// Get the color attachment texture ID (not the FBO handle)
		uint32_t textureID = frameData.editorResolvedBuffer->GetColorAttachmentRendererID(0);

		// Render the scene viewport using the texture ID
		ImGui::Image((ImTextureID)(uintptr_t)textureID,
			viewportSize, ImVec2(0, 1), ImVec2(1, 0));

		// CRITICAL: Query click state IMMEDIATELY after Image (while it's the "last item")
		// IsItemClicked() doesn't "consume" clicks - it just queries if this item was clicked
		bool imageClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
		bool viewportHovered = ImGui::IsItemHovered();

		// Drag-and-drop target for viewport (instantiate prefabs by dropping into 3D view)
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AssetDrop"))
			{
				const char* droppedPath = static_cast<const char*>(payload->Data);
				std::string assetPath(droppedPath);
				std::filesystem::path filepath(assetPath);

				// Check if dropped asset is a prefab
				if (filepath.extension().string() == ".prefab")
				{
					// TODO: Calculate 3D world position from mouse position using camera ray
					// For now, instantiate at origin
					engineService.ExecuteOnEngineThread([assetPath]() {
						auto world = Engine::GetWorld();
						PrefabData prefabData = PrefabSystem::LoadAndCachePrefab(assetPath.c_str());
						if (prefabData.IsValid()) {
							ecs::entity instantiated = PrefabSystem::InstantiatePrefab(world, prefabData.guid, glm::vec3(0.0f));
							if (instantiated) {
								spdlog::info("Prefab instantiated from viewport drag-and-drop: {}", prefabData.name);
							} else {
								spdlog::error("Failed to instantiate prefab from viewport: {}", prefabData.name);
							}
						}
					});
				}
			}
			ImGui::EndDragDropTarget();
		}

		// Now render the gizmo with proper viewport coordinates
		Gizmos(viewportPos, viewportSize);

		// Check if ImGuizmo wants input priority
		bool gizmoWantsInput = ImGuizmo::IsUsing() || ImGuizmo::IsOver();

		// Only perform viewport picking if image was clicked AND gizmo doesn't want input
		bool viewportClicked = imageClicked && !gizmoWantsInput;

		if (viewportClicked) {
			spdlog::info("Editor: Viewport clicked detected by ImGui");

			// Get mouse position relative to viewport
			ImVec2 mousePos = ImGui::GetMousePos();
			float relativeX = mousePos.x - viewportPos.x;
			float relativeY = mousePos.y - viewportPos.y;

			spdlog::info("Editor: Mouse pos ({:.0f}, {:.0f}), viewport pos ({:.0f}, {:.0f}), relative ({:.0f}, {:.0f})",
				mousePos.x, mousePos.y, viewportPos.x, viewportPos.y, relativeX, relativeY);

			// Ensure click is within viewport bounds
			if (relativeX >= 0 && relativeY >= 0 && relativeX < viewportSize.x && relativeY < viewportSize.y) {
				spdlog::info("Editor: Click is within viewport bounds - performing picking");
				PerformEntityPicking(relativeX, relativeY, viewportSize.x, viewportSize.y);
			}
			else {
				spdlog::warn("Editor: Click is outside viewport bounds");
			}
		}

		// Debug: Log mouse states
		static bool lastHovered = false;
		if (viewportHovered != lastHovered) {
			spdlog::info("Editor: Viewport hover state changed: {}", viewportHovered ? "entered" : "exited");
			lastHovered = viewportHovered;
		}

		// Handle camera input only when viewport is hovered and not clicking
		if (!m_IsPlayMode && m_EditorCamera && viewportHovered && !viewportClicked) {
			// Temporarily disable ImGui input capture for camera control
			ImGuiIO& io = ImGui::GetIO();
			bool originalWantCaptureMouse = io.WantCaptureMouse;
			bool originalWantCaptureKeyboard = io.WantCaptureKeyboard;

			//// Disable ImGui input capture only when doing camera control
			io.WantCaptureMouse = false;
			io.WantCaptureKeyboard = false;

			// Update camera based on input
			m_EditorCamera->Update(deltaTime);

			// Handle scroll for zoom
			double scrollX, scrollY;
			auto* input = InputManager::Get_Instance();
			input->Get_ScrollOffset(scrollX, scrollY);
			if (scrollY != 0) {
				m_EditorCamera->OnMouseScroll(static_cast<float>(scrollY));
			}

			// Restore original ImGui input capture state
			io.WantCaptureMouse = originalWantCaptureMouse;
			io.WantCaptureKeyboard = originalWantCaptureKeyboard;
		}
	}
	else {
		// Show placeholder text when no framebuffer is available
		ImGui::Text("Scene rendering not available - start engine render loop");
	}
	ImGui::End();
}

void EditorMain::CreateDefaultEntity()
{
	// FIXED: Execute on engine thread
	engineService.ExecuteOnEngineThread([]() {
		ecs::world world = Engine::GetWorld();

		// Create new entity
		auto entity = world.add_entity();

		// Add basic components
		world.add_component_to_entity<TransformComponent>(entity);
		world.add_component_to_entity<VisibilityComponent>(entity, true);
	});
}

void EditorMain::CreatePlaneEntity()
{
	// FIXED: Execute on engine thread
	engineService.ExecuteOnEngineThread([]() {
		ecs::world world = Engine::GetWorld();

		// Create new entity
		auto entity = world.add_entity();

		// Add transform components
		world.add_component_to_entity<TransformComponent>(entity, TransformComponent{ glm::vec3{ 1,1,1 }, glm::vec3{}, glm::vec3(0.0f, 0.0f, 0.0f) });
		world.add_component_to_entity<VisibilityComponent>(entity, true);


		// Add mesh renderer component
		MeshRendererComponent meshRenderer;
		meshRenderer.isPrimitive = true;
		meshRenderer.m_PrimitiveType = MeshRendererComponent::PrimitiveType::PLANE;
		meshRenderer.hasAttachedMaterial = false;
		meshRenderer.material.m_AlbedoColor = glm::vec3(0.8f, 0.3f, 0.3f);
		meshRenderer.material.metallic = 0.1f;
		meshRenderer.material.roughness = 0.8f;
		meshRenderer.material.m_MaterialGuid.m_guid = rp::null_guid; // Use 0 for default material

		world.add_component_to_entity<MeshRendererComponent>(entity, meshRenderer);

		// Add material overrides for customization (replaces embedded struct)
		MaterialOverridesComponent materialOverrides;
		materialOverrides.vec3Overrides["u_AlbedoColor"] = glm::vec3(0.8f, 0.3f, 0.3f); // Reddish color
		materialOverrides.floatOverrides["u_MetallicValue"] = 0.0f; // Non-metallic (dielectric)
		materialOverrides.floatOverrides["u_RoughnessValue"] = 0.9f; // Very rough (matte ground)
		world.add_component_to_entity<MaterialOverridesComponent>(entity, materialOverrides);
	});
}

void EditorMain::CreateLightEntity()
{
	// FIXED: Execute on engine thread
	engineService.ExecuteOnEngineThread([]() {
		ecs::world world = Engine::GetWorld();

		// Create new entity
		auto entity = world.add_entity();

		// Add position
		world.add_component_to_entity<TransformComponent>(entity, TransformComponent{ glm::vec3{ 1,1,1 }, glm::vec3{}, glm::vec3(0.0f, 5.0f, 0.0f) });

		// Add light component (default initializers handle most fields)
		LightComponent light;
		light.m_Type = Light::Type::Directional;
		light.m_Direction = glm::vec3(0.0f, -1.0f, 0.0f);
		light.m_Color = glm::vec3(1.0f, 1.0f, 1.0f);
		light.m_IsEnabled = true;
		light.m_Intensity = 1.f;
		light.m_Range = 10.0f;
		light.m_InnerCone = 30.0f;
		light.m_OuterCone = 45.0f;

		world.add_component_to_entity<LightComponent>(entity, light);
	});
}

void EditorMain::CreateCameraEntity()
{
	// FIXED: Execute on engine thread
	engineService.ExecuteOnEngineThread([]() {
		ecs::world world = Engine::GetWorld();

		// Create new entity
		auto entity = world.add_entity();

		// Add position component
		world.add_component_to_entity<TransformComponent>(entity, TransformComponent{ glm::vec3{ 1,1,1 }, glm::vec3{}, glm::vec3(0.0f, 2.0f, 5.0f) });

		// Add camera component
		CameraComponent camera;
		camera.m_Type = CameraComponent::CameraType::PERSPECTIVE;
		camera.m_IsActive = true;
		camera.m_Fov = 45.0f;
		camera.m_AspectRatio = 16.0f / 9.0f;
		camera.m_Near = 0.1f;
		camera.m_Far = 100.0f;
		camera.m_Up = glm::vec3(0.0f, 1.0f, 0.0f);
		camera.m_Right = glm::vec3(1.0f, 0.0f, 0.0f);
		camera.m_Front = glm::vec3(0.0f, 0.0f, -1.0f);
		camera.m_Yaw = -90.0f;  // Initially looking down -Z axis
		camera.m_Pitch = 0.0f;

		world.add_component_to_entity<CameraComponent>(entity, camera);
	});
}

void EditorMain::CreatePhysicsDemoScene()
{
	// FIXED: Execute entire physics demo creation on engine thread
	engineService.ExecuteOnEngineThread([this]() {
		ecs::world world = Engine::GetWorld();

		// Create new entity
		auto entity = world.add_entity();

		// Add transform components
		world.add_component_to_entity<TransformComponent>(entity, TransformComponent{ glm::vec3{ 1,1,1 }, glm::vec3{}, glm::vec3(0.0f, -2.0f, 0.0f) });
		world.add_component_to_entity<VisibilityComponent>(entity, true);

		// Add mesh renderer component
		MeshRendererComponent meshRenderer;
		meshRenderer.isPrimitive = true;
		meshRenderer.m_PrimitiveType = MeshRendererComponent::PrimitiveType::PLANE;
		meshRenderer.hasAttachedMaterial = false;
		meshRenderer.material.m_AlbedoColor = glm::vec3(0.8f, 0.3f, 0.3f);
		meshRenderer.material.metallic = 0.1f;
		meshRenderer.material.roughness = 0.8f;
		meshRenderer.material.m_MaterialGuid.m_guid = rp::null_guid; // Use 0 for default material

		world.add_component_to_entity<MeshRendererComponent>(entity, meshRenderer);

		// Add material overrides for customization (replaces embedded struct)
		MaterialOverridesComponent materialOverrides;
		materialOverrides.vec3Overrides["u_AlbedoColor"] = glm::vec3(0.8f, 0.3f, 0.3f); // Reddish color
		materialOverrides.floatOverrides["u_MetallicValue"] = 0.0f; // Non-metallic (dielectric)
		materialOverrides.floatOverrides["u_RoughnessValue"] = 0.9f; // Very rough (matte ground)
		world.add_component_to_entity<MaterialOverridesComponent>(entity, materialOverrides);

		auto& transform = world.get_component_from_entity<TransformComponent>(entity);
		transform.m_Scale = glm::vec3{ 50,1,50 };
		auto Trans = &world.get_component_from_entity<TransformMtxComponent>(entity);

		const auto& scale = transform.m_Scale;
		const auto& rot = transform.m_Rotation;
		const auto& pos = transform.m_Translation;

		glm::mat4 Rx = glm::rotate(glm::mat4(1.0f), (rot.x), glm::vec3(1, 0, 0));
		glm::mat4 Ry = glm::rotate(glm::mat4(1.0f), (rot.y), glm::vec3(0, 1, 0));
		glm::mat4 Rz = glm::rotate(glm::mat4(1.0f), (rot.z), glm::vec3(0, 0, 1));
		glm::mat4 R = Rz * Ry * Rx; // Note: rotation order ZYX
		Trans->m_Mtx = glm::translate(glm::mat4(1.0f), pos) * R * glm::scale(glm::mat4(1.0f), scale);



		// Creating Floor Physics
		JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(50.f, 0.1f, 50.f));
		floor_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.
		JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
		JPH::ShapeRefC floor_shape = floor_shape_result.Get();
		JPH::BodyCreationSettings floor_settings(floor_shape, JPH::Vec3(0.0f, -2.1f, 0.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Kinematic, Layers::NON_MOVING);
		floorplan = PhysicsSystem::Instance().GetBodyInterface().CreateBody(floor_settings);
		PhysicsSystem::Instance().GetBodyInterface().AddBody(floorplan->GetID(), JPH::EActivation::DontActivate);



		// Cube creation
		auto CPos = glm::vec3(1.0f, 22.0f, 0.0f);
		auto Cscale = glm::vec3(1.0f);
		auto Ccolor = glm::vec3(1.0f, 0.0f, 0.0f);
		auto entity2 = world.add_entity();

		// Add transform components
		world.add_component_to_entity<TransformComponent>(entity2, TransformComponent{ Cscale, glm::vec3{}, CPos });
		world.add_component_to_entity<VisibilityComponent>(entity2, true);
		world.add_component_to_entity<RigidBodyComponent>(entity2);

		// Use shared primitive mesh and default material
		rp::Guid meshGuid2{}; // Use 0 for primitive shared meshes
		rp::Guid materialGuid2{}; // Use 0 for default material

		// Add mesh renderer component
		MeshRendererComponent meshRenderer2;
		meshRenderer2.isPrimitive = true; // Mark as primitive for proper handling
		meshRenderer2.m_PrimitiveType = MeshRendererComponent::PrimitiveType::CUBE;
		meshRenderer2.m_MeshGuid.m_guid = meshGuid2;
		meshRenderer2.hasAttachedMaterial = false;
		meshRenderer2.m_MaterialGuid[0].m_guid = materialGuid2;

		world.add_component_to_entity<MeshRendererComponent>(entity2, meshRenderer2);

		// Add material overrides
		MaterialOverridesComponent materialOverrides2;
		materialOverrides2.vec3Overrides["u_AlbedoColor"] = Ccolor; // Use color directly (no multiplier to avoid clamping)
		materialOverrides2.floatOverrides["u_MetallicValue"] = 0.0f; // Non-metallic (dielectric materials like plastic/wood)
		materialOverrides2.floatOverrides["u_RoughnessValue"] = 0.7f; // Slightly rough for diffuse appearance
		world.add_component_to_entity<MaterialOverridesComponent>(entity2, materialOverrides2);
		auto RigidBody = &world.get_component_from_entity<RigidBodyComponent>(entity2);
		// Creating Cube

		JPH::BoxShapeSettings box_shape_settings(JPH::Vec3(0.5f, 0.5f, 0.5f));
		box_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.
		JPH::ShapeSettings::ShapeResult Box_shape_result = box_shape_settings.Create();
		JPH::ShapeRefC Box_shape = Box_shape_result.Get();
		JPH::BodyCreationSettings sphere_settings(Box_shape, PhysicsUtils::ToJolt(CPos), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
		sphere_id = PhysicsSystem::Instance().GetBodyInterface().CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);
		RigidBody->motionType = RigidBodyComponent::MotionType::Dynamic;
		//RigidBody->bodyID = sphere_id.GetIndexAndSequenceNumber();
	}); // End of ExecuteOnEngineThread lambda
}

void EditorMain::CreateDemoScene()
{
	spdlog::info("Creating demo scene with cubes and lighting");

	// Create 3x3 cube grid using local utilities
	CreateCubeGrid(3, 3.0f);

	// FIXED: Create light entity on engine thread
	engineService.ExecuteOnEngineThread([]() {
		ecs::world world = Engine::GetWorld();

		auto lightEntity = world.add_entity();
		world.add_component_to_entity<TransformComponent>(lightEntity, TransformComponent{ glm::vec3{1.f}, glm::vec3{}, glm::vec3(0.0f, 5.0f, 0.0f) });

		LightComponent light;
		light.m_Type = Light::Type::Directional;
		light.m_Direction = glm::vec3(0.2f, -0.8f, 0.3f);
		light.m_Color = glm::vec3(1.0f, 0.95f, 0.85f);
		light.m_Intensity = 1.0f;
		light.m_IsEnabled = true;
		light.m_Range = 10.0f;
		light.m_InnerCone = 30.0f;
		light.m_OuterCone = 45.0f;

		world.add_component_to_entity<LightComponent>(lightEntity, light);
	});

	// Set stronger ambient light for better visibility
	engineService.SetAmbientLight(glm::vec3(0.03f));

	spdlog::info("Demo scene created with 9 cubes and enhanced lighting");

	//CreatePhysicsDemoScene();

}

void EditorMain::CreatePhysicsCube()
{
	// FIXED: Execute on engine thread
	engineService.ExecuteOnEngineThread([this]() {
		ecs::world world = Engine::GetWorld();

		// Cube creation
		auto CPos = glm::vec3(1.0f, 22.0f, 0.0f);
		auto Cscale = glm::vec3(1.0f);
		auto Ccolor = glm::vec3(1.0f, 0.0f, 0.0f);
		auto entity2 = world.add_entity();

		// Add transform components
		world.add_component_to_entity<TransformComponent>(entity2, TransformComponent{ Cscale, glm::vec3{}, CPos });
		world.add_component_to_entity<VisibilityComponent>(entity2, true);
		world.add_component_to_entity<RigidBodyComponent>(entity2);

		// Use shared primitive mesh and default material
		rp::Guid meshGuid2{}; // Use 0 for primitive shared meshes
		rp::Guid materialGuid2{}; // Use 0 for default material

		// Add mesh renderer component
		MeshRendererComponent meshRenderer2;
		meshRenderer2.isPrimitive = true; // Mark as primitive for proper handling
		meshRenderer2.m_PrimitiveType = MeshRendererComponent::PrimitiveType::CUBE;
		meshRenderer2.m_MeshGuid.m_guid = meshGuid2;
		meshRenderer2.hasAttachedMaterial = false;
		meshRenderer2.m_MaterialGuid["unnamed slot"].m_guid = materialGuid2;

		world.add_component_to_entity<MeshRendererComponent>(entity2, meshRenderer2);

		// Add material overrides
		MaterialOverridesComponent materialOverrides2;
		materialOverrides2.vec3Overrides["u_AlbedoColor"] = Ccolor; // Use color directly (no multiplier to avoid clamping)
		materialOverrides2.floatOverrides["u_MetallicValue"] = 0.0f; // Non-metallic (dielectric materials like plastic/wood)
		materialOverrides2.floatOverrides["u_RoughnessValue"] = 0.7f; // Slightly rough for diffuse appearance
		world.add_component_to_entity<MaterialOverridesComponent>(entity2, materialOverrides2);
		auto RigidBody = &world.get_component_from_entity<RigidBodyComponent>(entity2);
		// Creating Cube

		BoxCollider BColl;
		//world.add_component_to_entity<BoxCollider>(entity2, BColl);
		JPH::BoxShapeSettings box_shape_settings(JPH::Vec3(0.5f, 0.5f, 0.5f));
		box_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.
		JPH::ShapeSettings::ShapeResult Box_shape_result = box_shape_settings.Create();
		JPH::ShapeRefC Box_shape = Box_shape_result.Get();
		JPH::BodyCreationSettings sphere_settings(Box_shape, PhysicsUtils::ToJolt(CPos), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
		sphere_id = PhysicsSystem::Instance().GetBodyInterface().CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);
		RigidBody->motionType = RigidBodyComponent::MotionType::Dynamic;
		//RigidBody->bodyID = sphere_id.GetIndexAndSequenceNumber();
	}); // End of ExecuteOnEngineThread lambda
}


void EditorMain::CreateCube(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& color)
{
	assert(scale.x > 0 && scale.y > 0 && scale.z > 0 && "Scale must be positive");

	// FIXED: Execute on engine thread
	engineService.ExecuteOnEngineThread([position, scale, color]() {
		auto world = Engine::GetWorld();
		auto entity = world.add_entity();

		// Add transform components
		world.add_component_to_entity<TransformComponent>(entity, TransformComponent{ scale, glm::vec3{}, position });

		world.add_component_to_entity<VisibilityComponent>(entity, true);

		// Use shared primitive mesh and default material
		rp::Guid meshGuid{}; // Use 0 for primitive shared meshes
		rp::Guid materialGuid{}; // Use 0 for default material

		// Add mesh renderer component
		MeshRendererComponent meshRenderer;
		meshRenderer.isPrimitive = true; // Mark as primitive for proper handling
		meshRenderer.m_PrimitiveType = MeshRendererComponent::PrimitiveType::CUBE;
		meshRenderer.m_MeshGuid.m_guid = meshGuid;
		meshRenderer.hasAttachedMaterial = false;
		meshRenderer.m_MaterialGuid["unnamed slot"].m_guid = materialGuid;

		world.add_component_to_entity<MeshRendererComponent>(entity, meshRenderer);

		// Add material overrides
		MaterialOverridesComponent materialOverrides;
		materialOverrides.vec3Overrides["u_AlbedoColor"] = color; // Use color directly (no multiplier to avoid clamping)
		materialOverrides.floatOverrides["u_MetallicValue"] = 0.0f; // Non-metallic (dielectric materials like plastic/wood)
		materialOverrides.floatOverrides["u_RoughnessValue"] = 0.7f; // Slightly rough for diffuse appearance
		world.add_component_to_entity<MaterialOverridesComponent>(entity, materialOverrides);
	});
}

void EditorMain::CreateCubeGrid(int gridSize, float spacing)
{
	assert(gridSize > 0 && gridSize <= 10 && "Grid size must be between 1-10");
	assert(spacing > 0.0f && "Spacing must be positive");

	// Vibrant colors for easy visual distinction
	std::vector<glm::vec3> colors = {
		glm::vec3(0.9f, 0.1f, 0.1f),  // Red - vibrant red
		glm::vec3(0.1f, 0.9f, 0.1f),  // Green - vibrant green
		glm::vec3(0.1f, 0.4f, 0.9f),  // Blue - vibrant blue
		glm::vec3(0.95f, 0.75f, 0.05f),  // Gold - bright gold
		glm::vec3(0.9f, 0.9f, 0.9f),  // Light gray - easier to see than white
	};

	const float startOffset = -(gridSize - 1) * spacing * 0.5f;

	for (int x = 0; x < gridSize; ++x) {
		for (int z = 0; z < gridSize; ++z) {
			glm::vec3 position(
				startOffset + x * spacing,
				0.0f,  // Ground level
				startOffset + z * spacing
			);

			// Cycle through materials based on position
			int colorIndex = (x + z) % colors.size();
			glm::vec3 color = colors[colorIndex];

			CreateCube(position, glm::vec3(1.0f), color);
		}
	}
}

void EditorMain::PerformEntityPicking(float mouseX, float mouseY, float viewportWidth, float viewportHeight)
{
	// FIXED: Pure encapsulation - all Engine API access in EngineService
	engineService.PerformEntityPicking(mouseX, mouseY, viewportWidth, viewportHeight,
		[this](bool hasHit, uint32_t objectID) {
		// Callback executes on engine thread, but only calls EditorMain methods
		if (hasHit) {
			objectID &= rp::lo_bitmask32_v<20>;
			auto& reg{ Engine::GetWorld().impl.get_registry() };
			SelectEntity(static_cast<std::uint32_t>(entt::entt_traits<entt::entity>::construct(objectID, reg.current(static_cast<entt::entity>(objectID)))));
		}
		else {
			ClearEntitySelection();
		}
	});
}

void EditorMain::SelectEntity(uint32_t objectID)
{
	m_SelectedEntityID = objectID;
	if (ImGui::IsKeyDown(ImGuiKey::ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey::ImGuiKey_RightCtrl)) {
		m_EntitiesIDSelection.emplace(objectID);
	}
	else {
		m_EntitiesIDSelection.clear();
		m_EntitiesIDSelection.emplace(objectID);
	}

	spdlog::info("Editor: Selected entity with Object ID: {}", m_SelectedEntityID);

	// FIXED: Pure encapsulation - all Engine API access in EngineService
	engineService.SelectEntityByObjectID(objectID);

	// Add visual feedback: outline the selected entity
	engineService.ClearOutlinedObjects();  // Clear previous selection
	for (auto const& objID : m_EntitiesIDSelection) {
		engineService.AddOutlinedObject(objID);  // Outline new selection
	}
}

void EditorMain::ClearEntitySelection()
{
	if (m_SelectedEntityID != 0) {
		spdlog::info("Editor: Cleared entity selection (was Object ID: {})", m_SelectedEntityID);
		m_SelectedEntityID = 0;

		// Clear visual feedback
		engineService.ClearOutlinedObjects();
	}
}

void EditorMain::HandleViewportPicking()
{
	// This function can be called to handle other picking-related logic
	// For now, picking is handled directly in Render_Scene()
}

void EditorMain::SaveScene(const char* path)
{
	// FIXED: Pure encapsulation - all Engine API access in EngineService
	engineService.SaveScene(path);
}


void EditorMain::LoadScene(const char* path)
{
	ClearEntitySelection();
	// FIXED: Pure encapsulation - all Engine API access in EngineService
	engineService.LoadScene(path);
	// Clear selection after loading new scene
}

void EditorMain::NewScene()
{
	ClearEntitySelection();
	// FIXED: Pure encapsulation - all Engine API access in EngineService
	engineService.NewScene();
	// Clear selection after loading new scene
}

void EditorMain::Gizmos(ImVec2 viewportPos, ImVec2 viewportSize)
{
	ImGui::Begin("Gizmo Debug");
	ImGuiIO& io = ImGui::GetIO();
	ImGui::Text("Selected Entity: %llu", m_SelectedEntityID);
	ImGui::Text("Mode: %d", (int)mode);
	ImGui::Text("Transform Valid: %s", GuizmoEntityTransform ? "YES" : "NO");
	ImGui::Text("TransformMtx Valid: %s", GuizmoEntityTransformMTX ? "YES" : "NO");
	ImGui::Text("Window Hovered: %s", ImGui::IsWindowHovered() ? "YES" : "NO");
	ImGui::Text("Gizmo Over: %s", ImGuizmo::IsOver() ? "YES" : "NO");
	ImGui::Text("Gizmo Using: %s", ImGuizmo::IsUsing() ? "YES" : "NO");
	ImGui::Text("Gizmo Capture Mouse: %s", io.WantCaptureMouse ? "YES" : "NO");
	ImGui::Text("Viewport Pos: (%.0f, %.0f)", viewportPos.x, viewportPos.y);
	ImGui::Text("Viewport Size: (%.0f, %.0f)", viewportSize.x, viewportSize.y);
	if (ImGui::Button("Test Scripts"))
	{
		engineService.ExecuteOnEngineThread([&]() {
			spdlog::info("enter test");
			ecs::world world = Engine::GetWorld();
			for (auto& entity : world.get_all_entities()) {
				if (entity.get_uid() == m_SelectedEntityID) {
					world.get_component_from_entity<BoxCollider>(entity).onTriggerEnter = [](const TriggerInfo&) {spdlog::critical("Hit");};
					spdlog::info("Found");
					break;
				}
			}
			spdlog::info("Add Trigger");
			});
	}

	ImGui::End();

	// This is for toggling the gizmo
	// Only process shortcuts when camera is NOT in free-look mode (right-click held)
	auto* input = InputManager::Get_Instance();
	bool isCameraActive = input->Is_MousePressed(GLFW_MOUSE_BUTTON_RIGHT);

	if (!isCameraActive)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Q, false))
		{
			mode = (ImGuizmo::OPERATION)0;
		}

		if (ImGui::IsKeyPressed(ImGuiKey_W, false))
		{
			mode = ImGuizmo::OPERATION::TRANSLATE;
		}

		if (ImGui::IsKeyPressed(ImGuiKey_E, false))
		{
			mode = ImGuizmo::OPERATION::ROTATE;
		}

		if (ImGui::IsKeyPressed(ImGuiKey_R, false))
		{
			mode = ImGuizmo::OPERATION::SCALE;
		}
	}

	// Only display gizmos if there is an entity selected and the gizmo is set to one of the 3 active modes
	if ((m_SelectedEntityID != 0) && (mode != (ImGuizmo::OPERATION)0))
	{

		// CRITICAL FIX: Use viewport content area position, not window title bar position
		ImGuizmo::SetOrthographic(true);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

		auto current_camera = CameraSystem::GetActiveCamera();

		// Grabbing the transform we want to edit
		/*const auto& entityHandles = engineService.GetEntitiesSnapshot();
		const auto& entityNames = engineService.get_reflectible_component_id_name_list();*/

		engineService.ExecuteOnEngineThread([&]() {
			auto world = Engine::GetWorld();
			if (m_SelectedEntityID) {
				auto selected = world.impl.entity_cast(entt::entity(m_SelectedEntityID));
				GuizmoEntityTransform = selected.all<TransformComponent>() ? &world.get_component_from_entity<TransformComponent>(selected) : nullptr;
				GuizmoEntityTransformMTX = selected.all<TransformComponent>() ? &world.get_component_from_entity<TransformMtxComponent>(selected) : nullptr;
				GuizmoEntityParentTransformMTX = SceneGraph::HasParent(selected) ? SceneGraph::GetParent(selected).all<TransformMtxComponent>() ? &SceneGraph::GetParent(selected).get<TransformMtxComponent>() : nullptr : nullptr;
			}
			});


		if (GuizmoEntityTransform != nullptr && GuizmoEntityTransformMTX != nullptr)
		{
			glm::mat4 deltas{};
			//glm::mat4 transmtx{ GuizmoEntityTransformMTX->m_Mtx }; 
			// Create and display Gizmos
			ImGuizmo::Manipulate(glm::value_ptr(GuizmoViewMec4), glm::value_ptr(GuizmoprojectionMat4), mode, ImGuizmo::LOCAL, glm::value_ptr(GuizmoEntityTransformMTX->m_Mtx));

			
			if (ImGuizmo::IsUsing()) // While we are using the gizmos
			{
				// Break down the edited matrix so we can save the values
				ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(GuizmoEntityParentTransformMTX ? glm::inverse(GuizmoEntityParentTransformMTX->m_Mtx) * GuizmoEntityTransformMTX->m_Mtx : GuizmoEntityTransformMTX->m_Mtx), glm::value_ptr(GuizmoEntityTransform->m_Translation), glm::value_ptr(GuizmoEntityTransform->m_Rotation), glm::value_ptr(GuizmoEntityTransform->m_Scale));
				std::cout << GuizmoEntityTransform->m_Translation.x;
				//GuizmoEntityTransformMTX->m_Mtx = transmtx;
				//isEditing = true; // Indicate that we are editing stuff
				//EditingID = selectedEnitityID; // Set the Id
				//if (HasBeenEditied == false) // Only Situation that can cause a problem is if you manage to edit two entities back to back
				//{
				//	AddToUndoStack(EditingID);
				//}
			}
		}



	}
	else
	{
		GuizmoEntityTransform = nullptr;
		GuizmoEntityTransformMTX = nullptr;
	}

	// ========================================================================
	// VIEW CUBE - Unity-style camera orientation widget
	// ========================================================================
	if (showViewCube) {
		// Draw view cube in top right corner
		float viewCubeSize = 128.0f;
	float viewCubePadding = 10.0f;
	ImVec2 viewCubePos = ImVec2(viewportPos.x + viewportSize.x - viewCubeSize - viewCubePadding,viewportPos.y + viewCubePadding);

	// Set up for view manipulator (always active, regardless of entity selection)
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

	// Determine if we should orbit around selected entity (LOCAL mode) or rotate in place (WORLD mode)
	bool isLocalMode = (mode == ImGuizmo::OPERATION::TRANSLATE || mode == ImGuizmo::OPERATION::ROTATE || mode == ImGuizmo::OPERATION::SCALE) && (m_SelectedEntityID != 0);
	glm::vec3 orbitTarget = glm::vec3(0.0f);

	if (isLocalMode && GuizmoEntityTransform != nullptr) {
		// LOCAL MODE: Orbit around selected entity
		orbitTarget = GuizmoEntityTransform->m_Translation;
	}
	// else: WORLD MODE - will rotate camera in place

	// Build view matrix for ViewManipulate
	glm::mat4 originalViewMatrix = m_EditorCamera->GetViewMatrix();
	glm::mat4 viewMatrix = originalViewMatrix;

	// Calculate distance for ViewManipulate
	glm::vec3 cameraPos = m_EditorCamera->GetPosition();
	float cameraDistance = isLocalMode ? glm::length(cameraPos - orbitTarget) : 10.0f;
	if (cameraDistance < 0.1f) cameraDistance = 10.0f;

	// ViewManipulate modifies the view matrix when user clicks on the cube
	ImGuizmo::ViewManipulate(glm::value_ptr(viewMatrix), cameraDistance, viewCubePos, ImVec2(viewCubeSize, viewCubeSize), 0x10101010);

	//// Check if the view matrix was modified
	bool viewMatrixChanged = glm::any(glm::notEqual(viewMatrix[0], originalViewMatrix[0])) ||
	                         glm::any(glm::notEqual(viewMatrix[1], originalViewMatrix[1])) ||
	                         glm::any(glm::notEqual(viewMatrix[2], originalViewMatrix[2])) ||
	                         glm::any(glm::notEqual(viewMatrix[3], originalViewMatrix[3]));

	if (viewMatrixChanged) {
	//	if (isLocalMode) {
	//		// LOCAL MODE: Camera orbits around selected entity
			// Extract new camera position from modified view matrix
			//glm::mat4 cameraTransform = glm::inverse(viewMatrix);
			//glm::vec3 newPosition = glm::vec3(cameraTransform[3]);

	//		// Update camera to look at the entity
			//m_EditorCamera->SetPosition(newPosition);
			//m_EditorCamera->SetTarget(orbitTarget);
	//	}
	//	else {
	//		// WORLD MODE: Camera rotates in place, doesn't move position
	//		// Extract rotation from the modified view matrix
	//		glm::mat4 cameraTransform = glm::inverse(viewMatrix);

	//		// Extract basis vectors (rotation only)
	//		glm::vec3 right = glm::normalize(glm::vec3(cameraTransform[0]));
	//		glm::vec3 up = glm::normalize(glm::vec3(cameraTransform[1]));
	//		glm::vec3 forward = -glm::normalize(glm::vec3(cameraTransform[2]));

	//		// Calculate pitch and yaw from forward vector
	//		float pitch = glm::degrees(asin(-forward.y));
	//		float yaw = glm::degrees(atan2(forward.x, forward.z));

	//		// Keep current position, only change rotation
	//		m_EditorCamera->SetRotation(glm::vec3(pitch, yaw, 0.0f));
	//	}
	}
	}  // End of if (showViewCube)
}
