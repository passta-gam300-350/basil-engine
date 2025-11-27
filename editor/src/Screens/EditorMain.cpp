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
		messagingSystem.Subscribe(CAMERA_CALCULATION_UPDATE, nullptr, [&](std::unique_ptr<Message> GuizmoMessage) {
			GuizmoViewMec4 = static_cast<Camera_Calculation_Update*>(GuizmoMessage.get())->viewMat4;
			GuizmoprojectionMat4 = static_cast<Camera_Calculation_Update*>(GuizmoMessage.get())->projectionMat4;
			});
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
	PF_EDITOR_SCOPE("WaitForEngineSnapshot");
	std::lock_guard lg{ engineService.m_cont->m_mtx }; //wait for snapshot
}

void EditorMain::render()
{
	if (!active) return;

	static uint64_t editorFrameIndex = 0;
	PF_EDITOR_BEGIN_FRAME(editorFrameIndex++);

	{
		PF_EDITOR_SCOPE("EditorUIWork");

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
		if (showEngineConsole)
			Render_EngineConsole();
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
	}

	{
		PF_EDITOR_SCOPE("ReleaseEngine");
		engineService.m_cont->m_container_is_presentable.release();
		engineService.start();
	}

	PF_EDITOR_END_FRAME();
}



void EditorMain::cleanup()
{
	engineService.ExecuteOnEngineThread([]() {
		PhysicsSystem::Instance().Exit();
		spdlog::info("Physics Exit");
		});
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
	colors[ImGuiCol_TitleBg] = ImVec4(0.040f, 0.040f, 0.040f, 1.000f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.040f, 0.040f, 0.040f, 1.000f);
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
	colors[ImGuiCol_Tab] = ImVec4(0.040f, 0.040f, 0.040f, 1.000f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.337f, 0.612f, 0.839f, 1.00f);
	colors[ImGuiCol_TabSelected] = ImVec4(0.235f, 0.235f, 0.235f, 1.00f);
	colors[ImGuiCol_TabDimmed] = ImVec4(0.040f, 0.040f, 0.040f, 1.000f);
	colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.235f, 0.235f, 0.235f, 1.00f);
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

	if (ImGui::BeginMenu("Effects")) {
		if (ImGui::MenuItem("Particle System")) {
			CreateParticleSystem();
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Light")) {
		if (ImGui::MenuItem("Directional Light")) {
			CreateLightEntity();
		}
		if (ImGui::MenuItem("Point Light")) {

		}
		if (ImGui::MenuItem("Spot Light")) {

		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Audio")) {
		if (ImGui::MenuItem("Audio Source")) {
			CreateAudioSource();
		}
		ImGui::EndMenu();
	}

	if (ImGui::MenuItem("Camera")) {
		CreateCameraEntity();
	}
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

