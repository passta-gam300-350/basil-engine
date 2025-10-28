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

#include "Physics/Physics_System.h"
#include <Messaging/Messaging_System.h>
#define UNREF_PARAM(x) x;

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

	// Note: ImGui callbacks are already set up in main.cpp
	// We need to chain our input handling with ImGui's callbacks
	// InputManager will be updated manually in render() to avoid conflicts
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
	m_AssetManager.reset(nullptr);
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
	}
	else {
		ImGui::Text("No entity selected");
		ImGui::Text("Click on an entity in the Scene viewport");
		ImGui::Text("or Hierarchy to select it.");
	}

	ImGui::End();
}

void EditorMain::Render_Components()
{
	auto& component_list{ engineService.m_cont->m_component_list_snapshot };
	auto& type_map{ ReflectionRegistry::types() };
	auto& internal_type_map{ ReflectionRegistry::InternalID() };

	static const ReflectionRegistry::TypeID skip_name_component{ internal_type_map[entt::type_index<ecs::entity::entity_name_t>::value()] };

	for (auto const& [type_id, uptr] : component_list) {
		if (type_id == skip_name_component) {
			continue;
		}
		entt::meta_type type = type_map[type_id];
		entt::meta_any comp = type.from_void(uptr.get());
		if (ImGui::TreeNode(ReflectionRegistry::GetTypeName(type.id()).c_str())) {
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
				if (meta_type.size_of() <= sizeof(int))
				{
					int enum_value = *static_cast<const int*>(val_ptr);
					ImGui::InputInt(field_name.c_str(), &enum_value);
				}
				else
				{
					std::cerr << "Unsupported enum underlying type size: " << meta_type.size_of() << " bytes\n";
				}
			}

			if (Resource::Guid* v = value.try_cast<Resource::Guid>())
				ImGui::Text((field_name + v->to_hex_no_delimiter()).c_str());


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
		}
		ImGui::PopID();
	}
}

void EditorMain::Render_Add_Component_Menu()
{
	auto& reflectible_component_list = engineService.get_reflectible_component_id_name_list();
	static const ReflectionRegistry::TypeID skip_name_component{ ReflectionRegistry::InternalID()[entt::type_index<ecs::entity::entity_name_t>::value()] };
	for (auto const&[type_id, type_name] : reflectible_component_list) {
		if (type_id == skip_name_component) {
			continue;
		}
		if (ImGui::MenuItem(type_name.c_str())) {
			
		}
	}
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
		ImGui::MenuItem("New Scene", "Ctrl+N");
		if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
		{
			std::string path{};
			if (fileService.OpenFileDialog(Editor::GetInstance().GetConfig().project_workingDir.c_str(), path, FileService::FILE_TYPE_LIST{ {L"Scene Files", L"*.scene"} }))
			{
				LoadScene(path.c_str());
				glfwSetWindowTitle(window, (Editor::GetInstance().GetConfig().workspace_name + " | " + std::filesystem::path{path}.filename().string()).c_str());

				
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
			fileService.SaveFileDialog(Editor::GetInstance().GetConfig().project_workingDir.c_str(), scenePath, FileService::FILE_TYPE_LIST{{L"Scene Files", L"*.scene"} });
			if (!scenePath.empty()) {
				
				SaveScene(scenePath.c_str());
			} else
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

	if (ImGui::BeginMenu("Help"))
	{
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

	// List all entities in the world
	ecs::world world = Engine::GetWorld();

	// Get all entities with position component (basic entities)
	auto entities = engineService.m_cont->m_entities_snapshot;

	ImGui::Text("Entities in scene:");
	for (auto ehdl : entities) {
		ecs::entity entity{ehdl};
		ImGui::PushID(static_cast<int>(entity.get_uid()));

		// Check if this entity is currently selected
		uint32_t entityUID = static_cast<uint32_t>(entity.get_uid());
		bool isSelected = (m_SelectedEntityID == entityUID);

		// Display entity info with selection highlighting
		std::string entityName = entity.name();

		// Check what components this entity has
		bool hasTransform = world.has_all_components_in_entity<TransformComponent>(entity);
		bool hasMesh = world.has_all_components_in_entity<MeshRendererComponent>(entity);
		bool hasLight = world.has_all_components_in_entity<LightComponent>(entity);
		bool hasVisibility = world.has_all_components_in_entity<VisibilityComponent>(entity);

		UNREF_PARAM(hasTransform);
		
		if (hasLight) entityName += " (Light)";
		else if (hasMesh) entityName += " (Mesh)";
		else entityName += " (Empty)";

		// Highlight selected entity with different color
		if (isSelected) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow text for selected
		}

		if (ImGui::TreeNode(entityName.c_str())) {
			// Show component info
			if (hasVisibility) {
				auto vis = world.get_component_from_entity<VisibilityComponent>(entity);
				ImGui::Text("Visible: %s", vis.m_IsVisible ? "Yes" : "No");
			}

			// Delete button
			if (ImGui::Button("Delete Entity")) {
				world.remove_entity(entity);
			}

			ImGui::TreePop();
		}

		// Pop the selection highlight color if it was applied
		if (isSelected) {
			ImGui::PopStyleColor();
		}

		// Allow clicking entity in hierarchy to select it
		if (ImGui::IsItemClicked()) {
			SelectEntity(entityUID);
		}

		ImGui::PopID();
	}

	ImGui::End();
}


void EditorMain::Render_Profiler()
{
	ImGui::Begin("Profiler");

	auto& info = Engine::Instance().GetInfo();

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
	// Example log messages
	ImGui::Text("Log Message 1");
	ImGui::Text("Log Message 2");
	ImGui::Text("Log Message 3");
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
	for (auto it = files.first; it != files.second; ++it) {
		std::filesystem::path filepath{ it->second.m_RawFileInfo.m_RawSourcePath };
		std::string filename = filepath.filename().string();

		if (filename.empty()) continue;

		ImGui::PushID(filename.c_str());
		ImGui::Button(filename.c_str(), { thumbnailSize, thumbnailSize }); // Creates a button for the folders
		if (ImGui::BeginPopupContextItem()) // if you right click on an asset
		{
			/*
			if (ImGui::MenuItem("Rename Asset")) // popup asking to rename asset
			{
				renameFileString = CurrentDirectory->directory_path.substr(CurrentDirectory->directory_path.find_first_of('/') + 1) + '\\' + FileName; // Save the filename you want to rename
				if (FileName.find_last_of('.') == std::string::npos)
				{
					renameFileExtention = "";
				}
				else
				{
					renameFileExtention = FileName.substr(FileName.find_last_of('.')); // save the file extension you want to rename
				}
				showPopup = true; // Start the rename pop up
			}
			if (ImGui::MenuItem("Delete Asset")) // popup asking to delete asset
			{
				deleteFileString = CurrentDirectory->directory_path.substr(CurrentDirectory->directory_path.find_first_of('/') + 1) + '\\' + FileName; // Save the filename you want to delete
				if (FileName.find_last_of('.') == std::string::npos)
				{
					deleteFileExtention = "";
				}
				else
				{
					deleteFileExtention = FileName.substr(FileName.find_last_of('.')); // save the file extension you want to delete
				}

				deleteFile = true; // starts the delete popup
			}*/
			if (ImGui::MenuItem("Import Asset")) // popup asking to import asset
			{
				m_AssetManager->ImportAsset(it->second);
			}
			ImGui::EndPopup();
		}

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

void EditorMain::Render_Scene()
{
	// Get delta time for camera updates
	float deltaTime = static_cast<float>(Engine::GetDeltaTime());

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
	auto& frameData = Engine::GetRenderSystem().m_SceneRenderer->GetFrameData();
	if (frameData.editorColorBuffer && frameData.editorColorBuffer->GetFBOHandle() != 0) {
		// Store viewport position for picking calculations
		ImVec2 viewportPos = ImGui::GetCursorScreenPos();

		// Get the color attachment texture ID (not the FBO handle)
		uint32_t textureID = frameData.editorColorBuffer->GetColorAttachmentRendererID(0);

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

	// Debug info below the viewport
	ecs::world world = Engine::GetWorld();
	auto entityCount = engineService.m_cont->m_entities_snapshot.size();
	auto meshCount = world.filter_entities<MeshRendererComponent>().size_hint();
	auto lightCount = world.filter_entities<LightComponent>().size_hint();
	auto cameraCount = world.filter_entities<CameraComponent>().size_hint();

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
	ImGui::Text("Editor FBO: %s", frameData.editorColorBuffer ? "Valid" : "None");
	if (frameData.editorColorBuffer) {
		ImGui::Text("FBO Handle: %u", frameData.editorColorBuffer->GetFBOHandle());
		ImGui::Text("Viewport Size: %.0fx%.0f", m_ViewportWidth, m_ViewportHeight);
	}

	ImGui::End();
}

void EditorMain::CreateDefaultEntity()
{
	ecs::world world = Engine::GetWorld();

	// Create new entity
	auto entity = world.add_entity();

	// Add basic components
	world.add_component_to_entity<TransformComponent>(entity);
	world.add_component_to_entity<VisibilityComponent>(entity, true);
}

void EditorMain::CreatePlaneEntity()
{
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
	meshRenderer.material.m_MaterialGuid = Resource::Guid{}; // Use 0 for default material

	world.add_component_to_entity<MeshRendererComponent>(entity, meshRenderer);
}

void EditorMain::CreateLightEntity()
{
	ecs::world world = Engine::GetWorld();

	// Create new entity
	auto entity = world.add_entity();

	// Add position
	world.add_component_to_entity<TransformComponent>(entity, TransformComponent{ glm::vec3{ 1,1,1 }, glm::vec3{}, glm::vec3(0.0f, 5.0f, 0.0f) });

	// Add light component
	LightComponent light;
	light.m_Type = Light::Type::Directional;
	light.m_Direction = glm::vec3(0.0f, -1.0f, 0.0f);
	light.m_Color = glm::vec3(1.0f, 1.0f, 1.0f);
	light.m_Intensity = 1.0f;
	light.m_Range = 10.0f;
	light.m_InnerCone = 30.0f;
	light.m_OuterCone = 45.0f;
	light.m_IsEnabled = true;

	world.add_component_to_entity<LightComponent>(entity, light);
}

void EditorMain::CreateCameraEntity()
{
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
}

void EditorMain::CreatePhysicsDemoScene()
{
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
	meshRenderer.material.m_MaterialGuid = Resource::Guid{}; // Use 0 for default material

	world.add_component_to_entity<MeshRendererComponent>(entity, meshRenderer);

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
	JPH::BodyCreationSettings floor_settings(floor_shape, JPH::Vec3(0.0f, -2.1f, 0.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);
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
	Resource::Guid meshGuid2{}; // Use 0 for primitive shared meshes
	Resource::Guid materialGuid2{}; // Use 0 for default material

	// Add mesh renderer component
	MeshRendererComponent meshRenderer2;
	meshRenderer2.isPrimitive = true; // Mark as primitive for proper handling
	meshRenderer2.m_PrimitiveType = MeshRendererComponent::PrimitiveType::CUBE;
	meshRenderer2.m_MeshGuid = meshGuid2;
	meshRenderer2.hasAttachedMaterial = false;
	meshRenderer2.material.m_MaterialGuid = materialGuid2;
	meshRenderer2.material.m_AlbedoColor = Ccolor * 1.2f; // Brighten the color slightly
	meshRenderer2.material.metallic = 0.0f; // Non-metallic for better color visibility
	meshRenderer2.material.roughness = 0.6f; // Medium roughness

	world.add_component_to_entity<MeshRendererComponent>(entity2, meshRenderer2);
	auto RigidBody = &world.get_component_from_entity<RigidBodyComponent>(entity2);
	// Creating Cube
	
	JPH::BoxShapeSettings box_shape_settings(JPH::Vec3(0.5f,0.5f,0.5f));
	box_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.
	JPH::ShapeSettings::ShapeResult Box_shape_result = box_shape_settings.Create();
	JPH::ShapeRefC Box_shape = Box_shape_result.Get();
	JPH::BodyCreationSettings sphere_settings(Box_shape, PhysicsUtils::ToJolt(CPos), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
	sphere_id = PhysSys.GetBodyInterface().CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);
	RigidBody->motionType = JPH::EMotionType::Dynamic;
	RigidBody->bodyID = sphere_id;

}

void EditorMain::CreateDemoScene()
{
	spdlog::info("Creating demo scene with cubes and lighting");

	// Create 3x3 cube grid using local utilities
	CreateCubeGrid(3, 3.0f);

	// Create additional directional light for better lighting
	ecs::world world = Engine::GetWorld();

	auto lightEntity = world.add_entity();
	world.add_component_to_entity<TransformComponent>(lightEntity, TransformComponent{ glm::vec3{1.f}, glm::vec3{}, glm::vec3(0.0f, 5.0f, 0.0f) });

	LightComponent light;
	light.m_Type = Light::Type::Directional;
	light.m_Direction = glm::vec3(0.2f, -0.8f, 0.3f);
	light.m_Color = glm::vec3(1.0f, 0.95f, 0.85f);
	light.m_Intensity = 1.0f;
	light.m_IsEnabled = true;

	world.add_component_to_entity<LightComponent>(lightEntity, light);

	// Set stronger ambient light for better visibility
	Engine::GetRenderSystem().m_SceneRenderer->SetAmbientLight(glm::vec3(0.3f, 0.3f, 0.3f));

	spdlog::info("Demo scene created with 9 cubes and enhanced lighting");

	//CreatePhysicsDemoScene();

}

void EditorMain::CreatePhysicsCube()
{
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
	Resource::Guid meshGuid2{}; // Use 0 for primitive shared meshes
	Resource::Guid materialGuid2{}; // Use 0 for default material

	// Add mesh renderer component
	MeshRendererComponent meshRenderer2;
	meshRenderer2.isPrimitive = true; // Mark as primitive for proper handling
	meshRenderer2.m_PrimitiveType = MeshRendererComponent::PrimitiveType::CUBE;
	meshRenderer2.m_MeshGuid = meshGuid2;
	meshRenderer2.hasAttachedMaterial = false;
	meshRenderer2.material.m_MaterialGuid = materialGuid2;
	meshRenderer2.material.m_AlbedoColor = Ccolor * 1.2f; // Brighten the color slightly
	meshRenderer2.material.metallic = 0.0f; // Non-metallic for better color visibility
	meshRenderer2.material.roughness = 0.6f; // Medium roughness

	world.add_component_to_entity<MeshRendererComponent>(entity2, meshRenderer2);
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
}


void EditorMain::CreateCube(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& color)
{
	assert(scale.x > 0 && scale.y > 0 && scale.z > 0 && "Scale must be positive");

	auto world = Engine::GetWorld();
	auto entity = world.add_entity();

	// Add transform components
	world.add_component_to_entity<TransformComponent>(entity, TransformComponent{ scale, glm::vec3{}, position });

	world.add_component_to_entity<VisibilityComponent>(entity, true);

	// Use shared primitive mesh and default material
	Resource::Guid meshGuid{}; // Use 0 for primitive shared meshes
	Resource::Guid materialGuid{}; // Use 0 for default material

	// Add mesh renderer component
	MeshRendererComponent meshRenderer;
	meshRenderer.isPrimitive = true; // Mark as primitive for proper handling
	meshRenderer.m_PrimitiveType = MeshRendererComponent::PrimitiveType::CUBE;
	meshRenderer.m_MeshGuid = meshGuid;
	meshRenderer.hasAttachedMaterial = false;
	meshRenderer.material.m_MaterialGuid = materialGuid;
	meshRenderer.material.m_AlbedoColor = color * 1.2f; // Brighten the color slightly
	meshRenderer.material.metallic = 0.0f; // Non-metallic for better color visibility
	meshRenderer.material.roughness = 0.6f; // Medium roughness


	world.add_component_to_entity<MeshRendererComponent>(entity, meshRenderer);
}

void EditorMain::CreateCubeGrid(int gridSize, float spacing)
{
	assert(gridSize > 0 && gridSize <= 10 && "Grid size must be between 1-10");
	assert(spacing > 0.0f && "Spacing must be positive");

	// Same colors as GraphicsTestDriver
	std::vector<glm::vec3> colors = {
		glm::vec3(0.8f, 0.2f, 0.2f),  // Red
		glm::vec3(0.2f, 0.8f, 0.2f),  // Green
		glm::vec3(0.2f, 0.2f, 0.8f),  // Blue
		glm::vec3(1.0f, 0.8f, 0.2f),  // Gold
		glm::vec3(1.0f, 1.0f, 1.0f),  // White
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
	auto* sceneRenderer = Engine::GetRenderSystem().m_SceneRenderer.get();
	assert(sceneRenderer && "SceneRenderer must be available for entity picking");

	// Assert input parameters are valid
	assert(viewportWidth > 0.0f && viewportHeight > 0.0f && "Viewport dimensions must be positive");
	assert(mouseX >= 0.0f && mouseY >= 0.0f && "Mouse coordinates must be non-negative");
	assert(mouseX < viewportWidth && mouseY < viewportHeight && "Mouse coordinates must be within viewport");

	spdlog::info("Editor: Performing entity picking at viewport position ({:.1f}, {:.1f}) in viewport ({:.1f}x{:.1f})",
		mouseX, mouseY, viewportWidth, viewportHeight);

	// Debug: Check how many entities exist and have renderable components
	ecs::world world = Engine::GetWorld();
	auto renderableCount = world.filter_entities<MeshRendererComponent, TransformComponent, VisibilityComponent>().size();

	auto totalEntities = engineService.m_cont->m_entities_snapshot.size();

	spdlog::info("Editor: World has {} total entities, {} with renderable components", totalEntities, renderableCount);

	// Assert we have entities to pick from
	assert(renderableCount > 0 && "No renderable entities found - nothing to pick");
	assert(totalEntities > 0 && "No entities found in world - check entity creation");

	// Enable picking pass temporarily
	sceneRenderer->EnablePicking(true);

	// Create picking query with viewport-relative coordinates
	MousePickingQuery query;
	query.screenX = static_cast<int>(mouseX);
	query.screenY = static_cast<int>(mouseY);
	query.viewportWidth = static_cast<int>(viewportWidth);
	query.viewportHeight = static_cast<int>(viewportHeight);

	// Assert picking system is properly configured
	auto* pipeline = sceneRenderer->GetPipeline();
	assert(pipeline && "SceneRenderer must have a valid pipeline");
	auto pickingPass = pipeline->GetPass("PickingPass");
	assert(pickingPass && "Pipeline must have a PickingPass for object picking");
	assert(pipeline->IsPassEnabled("PickingPass") && "PickingPass must be enabled for picking to work");

	// Perform picking query
	PickingResult result = sceneRenderer->QueryObjectPicking(query);

	// Assert picking query executed successfully
	assert(result.depth >= 0.0f && result.depth <= 1.0f && "Depth value must be in valid range [0,1]");

	// Disable picking pass after use
	sceneRenderer->EnablePicking(false);

	// Handle picking result
	if (result.hasHit && result.objectID != 0) {
		spdlog::info("Editor: Entity picked! Object ID: {}, World Position: ({:.2f}, {:.2f}, {:.2f})",
			result.objectID, result.worldPosition.x, result.worldPosition.y, result.worldPosition.z);
		SelectEntity(result.objectID);
	}
	else {
		spdlog::info("Editor: No entity picked - clearing selection");
		ClearEntitySelection();
	}
}

void EditorMain::SelectEntity(uint32_t objectID)
{
	m_SelectedEntityID = objectID;
	spdlog::info("Editor: Selected entity with Object ID: {}", m_SelectedEntityID);
	engineService.inspect_entity(ecs::entity(uint32_t(Engine::GetWorld()), objectID).get_uuid());
	// TODO: Add visual feedback for selected entity (highlight, outline, etc.)
	// This could involve setting a uniform or render state for the selected object
}

void EditorMain::ClearEntitySelection()
{
	if (m_SelectedEntityID != static_cast<uint32_t>(-1)) {
		spdlog::info("Editor: Cleared entity selection (was Object ID: {})", m_SelectedEntityID);
		m_SelectedEntityID = static_cast<uint32_t>(-1);
	}
}

void EditorMain::SetDebugVisualization(bool showAABBs)
{
	auto* sceneRenderer = Engine::GetRenderSystem().m_SceneRenderer.get();
	if (sceneRenderer == nullptr) {
		return;
	}

	auto* pipeline = sceneRenderer->GetPipeline();
	if (pipeline == nullptr) {
		return;
	}

	auto debugPass = std::dynamic_pointer_cast<DebugRenderPass>(pipeline->GetPass("DebugPass"));
	if (debugPass) {
		debugPass->SetShowAABBs(showAABBs);
	}
}

void EditorMain::HandleViewportPicking()
{
	// This function can be called to handle other picking-related logic
	// For now, picking is handled directly in Render_Scene()
}

void EditorMain::SaveScene(const char* path)
{
	if (path == nullptr) {
		spdlog::error("Editor: Cannot save scene - path is null");
		return;
	}
	ecs::world world = Engine::GetWorld();

	world.SaveYAML(path);
	spdlog::info("Editor: Scene saved to {}", path);

}


void EditorMain::LoadScene(const char* path)
{
	if (path == nullptr) {
		spdlog::error("Editor: Cannot load scene - path is null");
		return;
	}
	ClearEntitySelection();
	ecs::world world = Engine::GetWorld();
	world.UnloadAll();
	world.LoadYAML(path);
	spdlog::info("Editor: Scene loaded from {}", path);
	// Clear selection after loading new scene


}