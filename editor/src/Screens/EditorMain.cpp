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
#include <Resources/PrimitiveGenerator.h>
#include <Resources/Material.h>
#include <Input/InputManager.h>
#include <Pipeline/DebugRenderPass.h>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <chrono>

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
#define UNREF_PARAM(x) x;

RegisterImguiDescriptorInspector(ModelDescriptor);
RegisterImguiDescriptorInspector(TextureDescriptor);
RegisterImguiDescriptorInspector(MaterialDescriptor);

PhysicsSystem PhysSys;
JPH::Body* floorplan; // Delete this after m1
JPH::BodyID sphere_id;


EditorMain::EditorMain(GLFWwindow* _window) : Screen(_window)
{

}

void EditorMain::init()
{
	// Set window title
	glfwSetWindowTitle(window, (Editor::GetInstance().GetConfig().workspace_name + " | No Scene").c_str());
	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
	UNREF_PARAM(primaryMonitor);
	UNREF_PARAM(mode);
	// Set maximized
	glfwMaximizeWindow(window);

	m_AssetManager = std::make_unique<AssetManager>(Editor::GetInstance().GetConfig().project_workingDir + "/assets", Editor::GetInstance().GetConfig().project_workingDir + "/.imports");
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

	CreateDemoScene();

	SetupUnityStyle();
	//std::jthread jth(&Engine::Update);
	// Note: ImGui callbacks are already set up in main.cpp
	// We need to chain our input handling with ImGui's callbacks
	// InputManager will be updated manually in render() to avoid conflicts

	MonoEntityManager::GetInstance().Attach();
}


void EditorMain::update()
{
	if (!active) return;
	std::lock_guard lg{ engineService.m_cont->m_mtx }; //wait for snapshot
}

struct s2 {
	std::string some_value;
	bool is_true;
	glm::vec4 v4;
};

struct p {
	s2 struct2;
};

enum class testemum : std::uint8_t {
	pollo,
	water,
	enum1
};

struct test {
	int t1;
	double t2{ 3.14f };
	testemum enum_test;
	std::string t3;
	glm::vec3 vec3;
	std::vector<p> vector_of_ints;
	std::map<std::string, s2> map_of_str_bool;
};

test testa{};


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



	//glClearColor(1, 1, 1, 1);

	//// Update ECS camera entity with EditorCamera data BEFORE RenderSystem::Update
	//if (!m_IsPlayMode && m_EditorCamera) {
	//	ecs::world world = Engine::GetWorld();
	//	auto cameraEntities = world.filter_entities<CameraComponent, PositionComponent>();

	//	if (cameraEntities) {
	//		auto entity = *cameraEntities.begin();
	//		auto& cameraComponent = world.get_component_from_entity<CameraComponent>(entity);
	//		auto& positionComponent = world.get_component_from_entity<PositionComponent>(entity);

	//		// Update ECS camera with EditorCamera data
	//		positionComponent.m_WorldPos = m_EditorCamera->GetPosition();

	//		// DEBUG: Check what vectors EditorCamera produces and only update position
	//		glm::vec3 editorForward = m_EditorCamera->GetForward();
	//		glm::vec3 editorUp = m_EditorCamera->GetUp();
	//		glm::vec3 editorRight = m_EditorCamera->GetRight();


	//		// Update ECS camera vectors with EditorCamera data
	//		cameraComponent.m_Front = editorForward;
	//		cameraComponent.m_Up = editorUp;
	//		cameraComponent.m_Right = editorRight;
	//		cameraComponent.m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
	//	}
	//}

	//// Update engine systems
	//ecs::world world = Engine::GetWorld();
	//RenderSystem::System().Update(world);
	//Engine::EndFrame();
	//Engine::UpdateDebug();
	ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 4.0f);
	ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.157f, 0.157f, 0.157f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.235f, 0.235f, 0.235f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.337f, 0.612f, 0.839f, 1.0f));

	Render_MenuBar();

	if (showSceneExplorer)
		Render_SceneExplorer();
	if (showConsole)
		Render_Console();
	if (showProfiler)
		Render_Profiler();
	if (showInspector)
		Render_Inspector();
	Render_Scene();
	Render_Game();
	Render_CameraControls();
	Render_AssetBrowser();



	ImGui::Begin("TestReflection");
	ImguiInspectTypeRenderer::present(testa, "testreflectmenu");
	ImGui::End();

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

	PhysSys.Exit();
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
	if (m_SelectedEntityID != static_cast<uint32_t>(-1)) {
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

	if (auto it{ std::find_if(entities.begin(), entities.end(), [this](std::size_t ehdl) {return ecs::entity(ehdl).get_uid() == m_SelectedEntityID; }) }; it != entities.end()) {
		// Show entity components
		ImGui::Text("Entity UID: %llu", m_SelectedEntityID);

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
	std::lock_guard lg{ engineService.m_cont->m_mtx };
	auto& component_list{ engineService.m_cont->m_component_list_snapshot };
	auto& type_map{ ReflectionRegistry::types() };
	auto& internal_type_map{ ReflectionRegistry::InternalID() };

	static const ReflectionRegistry::TypeID skip_name_component{ internal_type_map[entt::type_index<ecs::entity::entity_name_t>::value()] };
	ReflectionRegistry::TypeID behaviour_component{};
	auto behaviourIt = internal_type_map.find(entt::type_index<behaviour>::value());
	if (behaviourIt != internal_type_map.end())
	{
		behaviour_component = behaviourIt->second;
	}

	for (auto const& [type_id, uptr] : component_list) {
		if (type_id == skip_name_component) {
			continue;
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
		if (ImGui::TreeNode(componentLabel)) {
			bool is_dirty = false;
			Render_Component_Member(comp, is_dirty);
			ImGui::TreePop();
			if (is_dirty) {
				engineService.m_cont->m_write_back_queue.push(type_id);
			}
		}
	}
}

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
		if (meta_type.data().begin() != meta_type.data().end()) {
			if (ImGui::CollapsingHeader(field_name.c_str())) {
				Render_Component_Member(value, is_dirty);
			}
		}
		else {
			if (meta_type.is_enum())
			{
				const void* val_ptr = value.base().data();

				// Properly read enum value based on its underlying type size
				int enum_value = 0;
				if (meta_type.size_of() == sizeof(uint8_t)) {
					enum_value = *static_cast<const uint8_t*>(val_ptr);
				}
				else if (meta_type.size_of() == sizeof(uint16_t)) {
					enum_value = *static_cast<const uint16_t*>(val_ptr);
				}
				else if (meta_type.size_of() == sizeof(uint32_t)) {
					enum_value = *static_cast<const uint32_t*>(val_ptr);
				}
				else {
					std::cerr << "Unsupported enum underlying type size: " << meta_type.size_of() << " bytes\n";
				}

				ImGui::InputInt(field_name.c_str(), &enum_value);
			}

			if (rp::BasicIndexedGuid* v = value.try_cast<rp::BasicIndexedGuid>()) {
				std::vector<std::string> assetnames = m_AssetManager->GetAssetTypeNames(v->m_typeindex);
				std::string currentselectionname = m_AssetManager->ResolveAssetName(*v);
				std::size_t type = v->m_typeindex;
				assetnames.emplace_back("");
				auto it{ std::find_if(assetnames.begin(), assetnames.end(), [currentselectionname](std::string const& a) {return currentselectionname == a; }) };
				if (it != assetnames.end()) {
					std::swap(*it, assetnames.front());
				}
				int current_item{};
				ImGui::Text(field_name.c_str());
				ImGui::SameLine(150);
				ImGui::SetNextItemWidth(-1);
				ImGui::Combo("##guid selector", &current_item, [](void* data, int idx, const char** out_text) {
					auto& vec = *static_cast<std::vector<std::string>*>(data);
					if (idx < 0 || idx >= vec.size()) return false;
					*out_text = vec[idx].c_str();
					return true;
				}, static_cast<void*>(&assetnames), static_cast<int>(assetnames.size()));
				if (current_item != 0) {
					*v = m_AssetManager->ResolveAssetGuid(assetnames[current_item]);
					v->m_typeindex = type;
					is_dirty = true;
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
						for (auto& [key, val] : *map_float) {
							ImGui::PushID(idx++);
							if (ImGui::InputFloat(key.c_str(), &val)) {
								is_dirty = true;
							}
							ImGui::PopID();
						}
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
						for (auto& [key, val] : *map_vec3) {
							ImGui::PushID(idx++);
							// Use Uint8 flag to display as 0-255 instead of 0.0-1.0
							if (ImGui::ColorEdit3(key.c_str(), &val.x, ImGuiColorEditFlags_Uint8)) {
								is_dirty = true;
							}
							ImGui::PopID();
						}
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
						for (auto& [key, val] : *map_vec4) {
							ImGui::PushID(idx++);
							// Use Uint8 flag to display as 0-255 instead of 0.0-1.0
							if (ImGui::ColorEdit4(key.c_str(), &val.x, ImGuiColorEditFlags_Uint8)) {
								is_dirty = true;
							}
							ImGui::PopID();
						}
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
						for (auto& [key, val] : *map_mat4) {
							ImGui::Text("%s: [mat4 - not editable]", key.c_str());
						}
					}
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
		}
		ImGui::PopID();
	}
}

void EditorMain::Render_Add_Component_Menu()
{
	auto& reflectible_component_list = engineService.get_reflectible_component_id_name_list();
	static const ReflectionRegistry::TypeID skip_name_component{ ReflectionRegistry::InternalID()[entt::type_index<ecs::entity::entity_name_t>::value()] };
	for (auto const& [type_id, type_name] : reflectible_component_list) {
		if (type_id == skip_name_component) {
			continue;
		}
		if (ImGui::MenuItem(type_name.c_str())) {
			if (type_name == "Behaviour")
			{
				ecs::entity entity{ static_cast<std::uint32_t>(Engine::GetWorld()), m_SelectedEntityID };
				entity.add<behaviour>();
			}
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
	// TODO: Implement Add Script Menu
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

		}
		else // Stops Game
		{
			//OnPlayStop();
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

	if (ImGui::BeginMenu("View"))
	{
		ImGui::MenuItem("Inspector", nullptr, &showInspector);
		ImGui::MenuItem("Scene Explorer", nullptr, &showSceneExplorer);
		ImGui::MenuItem("Profiler", nullptr, &showProfiler);
		ImGui::MenuItem("Console", nullptr, &showConsole);

		ImGui::Separator();

		if (ImGui::MenuItem("Show Bounding Boxes", nullptr, &m_ShowAABBs))
		{
			SetDebugVisualization(m_ShowAABBs);
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("GameObject"))
	{
		if (ImGui::MenuItem("Create Empty"))
		{
			// Create Entity with transform
		}

		ImGui::Separator();

		if (ImGui::BeginMenu("3D Object"))
		{
			if (ImGui::MenuItem("Cube"))
			{

			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Lights"))
		{
			if (ImGui::MenuItem("Point Light"))
			{

			}

			if (ImGui::MenuItem("Spot Light"))
			{

			}

			if (ImGui::MenuItem("Directional Light"))
			{

			}

			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Create Camera"))
		{
			// Create Entity with transform
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

void EditorMain::Render_SceneExplorer()
{

	ImGui::Begin("Hierarchy", &showSceneExplorer);

	// Search bar (Unity-style)
	/*
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
	static char searchBuffer[256] = "";
	ImGui::SetNextItemWidth(-1);
	ImGui::InputTextWithHint("##Search", "Search...", searchBuffer, sizeof(searchBuffer));
	ImGui::PopStyleVar();
	*/

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Scene tree
	ecs::world world = Engine::GetWorld();
	auto entities = world.filter_entities<PositionComponent>();

	ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 16.0f);

	if (ImGui::TreeNodeEx("Scene", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
	{
		int entityIndex = 0;
		for (auto& entity : entities)
		{
			uint32_t entityID = static_cast<uint32_t>(entity.get_uid());

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf |
				ImGuiTreeNodeFlags_NoTreePushOnOpen |
				ImGuiTreeNodeFlags_SpanAvailWidth;

			if (entityID == m_SelectedEntityID)
				flags |= ImGuiTreeNodeFlags_Selected;


			ImGui::TreeNodeEx((void*)(intptr_t)entityID, flags, "%s", entity.name().c_str());

			if (ImGui::IsItemClicked())
			{
				m_SelectedEntityID = entityID;
			}

			// Right-click context menu
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Duplicate")) { /* Duplicate entity */ }
				if (ImGui::MenuItem("Delete")) { /* Delete entity */ }
				ImGui::Separator();
				if (ImGui::MenuItem("Rename")) { /* Rename entity */ }
				ImGui::EndPopup();
			}

			entityIndex++;
		}

		ImGui::TreePop();
	}

	ImGui::PopStyleVar();

	// Right-click in empty space to create new objects
	if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::BeginMenu("Create"))
		{
			if (ImGui::MenuItem("Empty GameObject")) { /* Create empty */ }
			ImGui::Separator();
			if (ImGui::MenuItem("Cube")) { /* Create cube */ }
			if (ImGui::MenuItem("Sphere")) { /* Create sphere */ }
			if (ImGui::MenuItem("Plane")) { /* Create plane */ }
			ImGui::Separator();
			if (ImGui::MenuItem("Camera")) { /* Create camera */ }
			if (ImGui::MenuItem("Light")) { /* Create light */ }
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}

	ImGui::End();

	/*
	ImGui::Begin("Hierarchy");

	if (ImGui::CollapsingHeader("Create Entities")) {
		// Entity creation buttons
		if (ImGui::Button("Create Empty")) {
			CreateDefaultEntity();
		}
		//ImGui::SameLine();
		if (ImGui::Button("Create Plane")) {
			CreatePlaneEntity();
		}
		//ImGui::SameLine();
		if (ImGui::Button("Create Cube")) {
			CreateCube(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(1.0f), glm::vec3(0.5f, 0.5f, 1.0f));
		}
		//ImGui::SameLine();
		if (ImGui::Button("Create Light")) {
			CreateLightEntity();
		}
		//ImGui::SameLine();
		if (ImGui::Button("Create Camera")) {
			CreateCameraEntity();
		}

		if (ImGui::Button("Create Physics Cube"))
		{
			CreatePhysicsCube();
		}
	}


	ImGui::Separator();

	// FIXED: Use snapshot data instead of Engine::GetWorld()
	const auto& entityHandles = engineService.GetEntitiesSnapshot();
	const auto& entityNames = engineService.GetEntityNamesSnapshot();

	ImGui::Text("Entities in scene:");
	for (size_t i = 0; i < entityHandles.size(); ++i) {
		auto ehdl = entityHandles[i];
		ImGui::PushID(static_cast<int>(i));

		// Check if this entity is currently selected
		uint32_t entityUID = ecs::entity(ehdl).get_uid();
		bool isSelected = (m_SelectedEntityID == entityUID);

		// DEBUG: Log entity IDs for all entities
		static bool loggedOnce = false;
		if (!loggedOnce && i == 0) {
			spdlog::info("DEBUG: m_SelectedEntityID = {}", m_SelectedEntityID);
			for (size_t j = 0; j < entityHandles.size(); ++j) {
				uint32_t uid = static_cast<uint32_t>(entityHandles[j]);
				spdlog::info("DEBUG: Entity [{}] UID = {}, Name = {}", j, uid, entityNames[j]);
			}
			loggedOnce = true;
		}

		// Display entity info with selection highlighting
		std::string entityName = entityNames[i];

		// Check what components this entity has (using snapshot)
		bool hasTransform = engineService.EntityHasComponent(ehdl, ReflectionRegistry::GetTypeID<TransformComponent>());
		bool hasMesh = engineService.EntityHasComponent(ehdl, ReflectionRegistry::GetTypeID<MeshRendererComponent>());
		bool hasLight = engineService.EntityHasComponent(ehdl, ReflectionRegistry::GetTypeID<LightComponent>());
		bool hasVisibility = engineService.EntityHasComponent(ehdl, ReflectionRegistry::GetTypeID<VisibilityComponent>());

		UNREF_PARAM(hasTransform);

		if (hasLight) entityName += " (Light)";
		else if (hasMesh) entityName += " (Mesh)";
		else entityName += " (Empty)";

		// Highlight selected entity with different color
		if (isSelected) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow text for selected
		}

		bool nodeOpen = ImGui::TreeNode(entityName.c_str());

		// Check if TreeNode header was clicked (must be done immediately after TreeNode call)
		if (ImGui::IsItemClicked()) {
			spdlog::info("DEBUG: Entity clicked - index {}, entityUID = {}, entityName = {}", i, entityUID, entityName);
			SelectEntity(entityUID);
		}

		if (nodeOpen) {
			// Show component info
			if (hasVisibility) {
				ImGui::Text("Has Visibility Component");
				// Note: Detailed component data requires inspector (uses snapshot)
			}

			// Delete button
			if (ImGui::Button("Delete Entity")) {
				engineService.delete_entity(ehdl);
			}

			ImGui::TreePop();
		}

		// Pop the selection highlight color if it was applied
		if (isSelected) {
			ImGui::PopStyleColor();
		}

		ImGui::PopID();
	}

	ImGui::End();
	*/
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


void EditorMain::Render_Game()
{
	ImGui::Begin("Game");
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
		ImGui::BulletText("Fly Mode:");
		ImGui::Text("  Right Click + Drag: Look around");
		ImGui::Text("  WASD: Move horizontally");
		ImGui::Text("  Q/E: Move up/down");
		ImGui::Text("  Shift: Speed boost");
		ImGui::Text("  Scroll: Adjust move speed");

		ImGui::BulletText("Orbit Mode:");
		ImGui::Text("  Middle Click + Drag: Orbit");
		ImGui::Text("  Shift + Middle: Pan");
		ImGui::Text("  Scroll: Zoom in/out");

		ImGui::BulletText("Pan Mode:");
		ImGui::Text("  Middle Click + Drag: Pan");

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
				m_AssetManager->ImportAssetDirectory(subd);
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

		ImGui::PushID(filename.c_str());
		ImGui::Button(filename.c_str(), { thumbnailSize, thumbnailSize }); // Creates a button for the folders
		if (ImGui::BeginPopupContextItem()) // if you right click on an asset
		{
			if (ImGui::MenuItem("Import Asset")) // popup asking to import asset
			{
				m_AssetManager->ImportAsset(it->second);
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

void EditorMain::Render_Scene()
{
	// Get delta time for camera updates
	float deltaTime = static_cast<float>(engineService.GetDeltaTime());

	ImGui::Begin("Scene");

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

		// Get the color attachment texture ID (not the FBO handle)
		uint32_t textureID = frameData.editorResolvedBuffer->GetColorAttachmentRendererID(0);

		// Render the scene viewport using the texture ID
		ImGui::Image((ImTextureID)(uintptr_t)textureID,
			viewportSize, ImVec2(0, 1), ImVec2(1, 0));

		// Handle viewport picking - check if viewport was clicked
		bool viewportClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
		bool viewportHovered = ImGui::IsItemHovered();

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

			// Disable ImGui input capture only when doing camera control
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

	// Debug info below the viewport (using snapshot)
	const auto& entityHandles = engineService.GetEntitiesSnapshot();
	auto entityCount = entityHandles.size();

	// Count entities by component type
	size_t meshCount = 0, lightCount = 0, cameraCount = 0;
	for (auto ehdl : entityHandles) {
		if (engineService.EntityHasComponent(ehdl, ReflectionRegistry::GetTypeID<MeshRendererComponent>())) meshCount++;
		if (engineService.EntityHasComponent(ehdl, ReflectionRegistry::GetTypeID<LightComponent>())) lightCount++;
		if (engineService.EntityHasComponent(ehdl, ReflectionRegistry::GetTypeID<CameraComponent>())) cameraCount++;
	}

	ImGui::Text("Total entities: %d", entityCount);
	ImGui::Text("Mesh entities: %d", meshCount);
	ImGui::Text("Light entities: %d", lightCount);
	ImGui::Text("Camera entities: %d", cameraCount);

	// Debug camera info
	if (m_EditorCamera) {
		glm::vec3 pos = m_EditorCamera->GetPosition();
		glm::vec3 rot = m_EditorCamera->GetRotation();
		ImGui::Text("Editor Cam Pos: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
		ImGui::Text("Editor Cam Rot: %.2f, %.2f, %.2f", rot.x, rot.y, rot.z);

		// Debug input state
		auto* input = InputManager::Get_Instance();
		bool rightMousePressed = input->Is_MousePressed(GLFW_MOUSE_BUTTON_RIGHT);
		bool middleMousePressed = input->Is_MousePressed(GLFW_MOUSE_BUTTON_MIDDLE);
		bool wPressed = input->Is_KeyPressed(GLFW_KEY_W);

		// Debug scroll state
		double scrollX, scrollY;
		input->Get_ScrollOffset(scrollX, scrollY);

		ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("Right Mouse: %s, Middle Mouse: %s", rightMousePressed ? "Yes" : "No", middleMousePressed ? "Yes" : "No");
		ImGui::Text("W Key: %s", wPressed ? "Yes" : "No");
		ImGui::Text("Scroll Y: %.2f", scrollY);
		ImGui::Text("ImGui wants mouse: %s, keyboard: %s", io.WantCaptureMouse ? "Yes" : "No", io.WantCaptureKeyboard ? "Yes" : "No");
	}

	// Show framebuffer status
	ImGui::Text("Editor FBO: %s", frameData.editorResolvedBuffer ? "Valid" : "None");
	if (frameData.editorResolvedBuffer) {
		ImGui::Text("FBO Handle: %u", frameData.editorResolvedBuffer->GetFBOHandle());
		ImGui::Text("Viewport Size: %.0fx%.0f", m_ViewportWidth, m_ViewportHeight);
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
		floorplan = PhysSys.GetBodyInterface().CreateBody(floor_settings);
		PhysSys.GetBodyInterface().AddBody(floorplan->GetID(), JPH::EActivation::DontActivate);



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
		meshRenderer2.m_MaterialGuid.m_guid = materialGuid2;

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
		sphere_id = PhysSys.GetBodyInterface().CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);
		RigidBody->motionType = JPH::EMotionType::Dynamic;
		RigidBody->bodyID = sphere_id;
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
		meshRenderer2.m_MaterialGuid.m_guid = materialGuid2;

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
		sphere_id = PhysSys.GetBodyInterface().CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);
		RigidBody->motionType = JPH::EMotionType::Dynamic;
		RigidBody->bodyID = sphere_id;
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
		meshRenderer.m_MaterialGuid.m_guid = materialGuid;

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
			SelectEntity(objectID);
		}
		else {
			ClearEntitySelection();
		}
	});
}

void EditorMain::SelectEntity(uint32_t objectID)
{
	m_SelectedEntityID = objectID;
	spdlog::info("Editor: Selected entity with Object ID: {}", m_SelectedEntityID);

	// FIXED: Pure encapsulation - all Engine API access in EngineService
	engineService.SelectEntityByObjectID(objectID);

	// Add visual feedback: outline the selected entity
	engineService.ClearOutlinedObjects();  // Clear previous selection
	engineService.AddOutlinedObject(objectID);  // Outline new selection
}

void EditorMain::ClearEntitySelection()
{
	if (m_SelectedEntityID != static_cast<uint32_t>(-1)) {
		spdlog::info("Editor: Cleared entity selection (was Object ID: {})", m_SelectedEntityID);
		m_SelectedEntityID = static_cast<uint32_t>(-1);

		// Clear visual feedback
		engineService.ClearOutlinedObjects();
	}
}

void EditorMain::SetDebugVisualization(bool showAABBs)
{
	// FIXED: Pure encapsulation using EngineService
	engineService.EnableAABBVisualization(showAABBs);
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