// THIS IS THE ACTUAL LEVEL EDITOR!!!,

#include <Render/Render.h>
#include <Engine.hpp>
#include <components/transform.h>
#include <Resources/PrimitiveGenerator.h>
#include <Resources/Material.h>
#include <Input/InputManager.h>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <chrono>

#include "Screens/EditorMain.hpp"

#include "Editor.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "GLFW/glfw3.h"
#include "Profiler/profiler.hpp"

EditorMain::EditorMain(GLFWwindow* _window) : Screen(_window)
{

}

void EditorMain::init()
{
	// Set window title
	glfwSetWindowTitle(window, (Editor::GetInstance().GetConfig().workspace_name + " | No Scene").c_str());
	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
	// Set maximized
	glfwMaximizeWindow(window);


	// Set decoration on
	glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);

	// init engine
	Engine::GenerateDefaultConfig();
	Engine::InitWithoutWindow("Default.yaml");

	// Note: ImGui callbacks are already set up in main.cpp
	// We need to chain our input handling with ImGui's callbacks
	// InputManager will be updated manually in render() to avoid conflicts

	// Initialize Editor Camera
	m_EditorCamera = std::make_unique<EditorCamera>();
	m_EditorCamera->SetPosition(glm::vec3(0.0f, 5.0f, 10.0f));
	// Set rotation to look towards origin from (0,5,10): pitch down, yaw to face -Z direction
	m_EditorCamera->SetRotation(glm::vec3(-25.0f, -90.0f, 0.0f));
	// Set orbit target to origin so orbit mode works
	m_EditorCamera->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));

	// Create a basic light so we can see something
	CreateLightEntity();
	// Create an ECS camera entity for the RenderSystem to use
	// We'll update it with EditorCamera data each frame
	CreateCameraEntity();

	// Create demo scene with cubes for testing
	CreateDemoScene();

	//std::jthread jth(&Engine::Update);
}

void EditorMain::render()
{
	Engine::BeginFrame();
	// Menu Bar
	if (!active) return;

	// Update input manager ONCE per frame - at the very beginning
	auto* input = InputManager::Get_Instance();
	input->Update();






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



	glClearColor(1, 1, 1, 1);

	// Update ECS camera entity with EditorCamera data BEFORE RenderSystem::Update
	if (!m_IsPlayMode && m_EditorCamera) {
		ecs::world world = Engine::GetWorld();
		auto cameraEntities = world.filter_entities<CameraComponent, PositionComponent>();

		if (cameraEntities) {
			auto entity = *cameraEntities.begin();
			auto& cameraComponent = world.get_component_from_entity<CameraComponent>(entity);
			auto& positionComponent = world.get_component_from_entity<PositionComponent>(entity);

			// Update ECS camera with EditorCamera data
			positionComponent.m_WorldPos = m_EditorCamera->GetPosition();

			// DEBUG: Check what vectors EditorCamera produces and only update position
			glm::vec3 editorForward = m_EditorCamera->GetForward();
			glm::vec3 editorUp = m_EditorCamera->GetUp();
			glm::vec3 editorRight = m_EditorCamera->GetRight();


			// Update ECS camera vectors with EditorCamera data
			cameraComponent.m_Front = editorForward;
			cameraComponent.m_Up = editorUp;
			cameraComponent.m_Right = editorRight;
			cameraComponent.m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
		}
	}

	// Update engine systems
	ecs::world world = Engine::GetWorld();
	RenderSystem::System().Update(world);
	Engine::EndFrame();
	Engine::UpdateDebug();

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



}

void EditorMain::cleanup()
{

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

		// Try to find the entity in the world
		ecs::world world = Engine::GetWorld();
		auto entities = world.filter_entities<PositionComponent>();

		bool entityFound = false;
		for (auto entity : entities) {
			if (static_cast<uint32_t>(entity.get_uid()) == m_SelectedEntityID) {
				entityFound = true;

				// Show entity components
				ImGui::Text("Entity UID: %llu", entity.get_uid());

				//// Position Component
				//if (world.has_all_components_in_entity<PositionComponent>(entity)) {
				//	auto& pos = world.get_component_from_entity<PositionComponent>(entity);
				//	ImGui::Text("Position Component:");
				//	ImGui::Text("  World Pos: (%.2f, %.2f, %.2f)", pos.m_WorldPos.x, pos.m_WorldPos.y, pos.m_WorldPos.z);
				//}

				//// Transform Component
				//if (world.has_all_components_in_entity<TransformComponent>(entity)) {
				//	ImGui::Text("Transform Component: Present");
				//}
				Render_Transform_Group_Component(entity);

				// Mesh Renderer Component
				/*if (world.has_all_components_in_entity<MeshRendererComponent>(entity)) {
					auto& meshRenderer = world.get_component_from_entity<MeshRendererComponent>(entity);
					ImGui::Text("Mesh Renderer Component:");
					ImGui::Text("  Mesh GUID: %s", meshRenderer.m_MeshGuid.to_hex().c_str());
					ImGui::Text("  Material GUID: %s", meshRenderer.m_MaterialGuid.to_hex().c_str());
				}*/
				Render_Mesh_Component(entity);

				//// Light Component
				//if (world.has_all_components_in_entity<LightComponent>(entity)) {
				//	auto& light = world.get_component_from_entity<LightComponent>(entity);
				//	ImGui::Text("Light Component:");
				//	ImGui::Text("  Type: %d", static_cast<int>(light.m_Type));
				//	ImGui::Text("  Color: (%.2f, %.2f, %.2f)", light.m_Color.r, light.m_Color.g, light.m_Color.b);
				//	ImGui::Text("  Intensity: %.2f", light.m_Intensity);
				//	ImGui::Text("  Enabled: %s", light.m_IsEnabled ? "Yes" : "No");
				//}

				Render_Lighting_Group_Component(entity);

				// Camera Component
				/*if (world.has_all_components_in_entity<CameraComponent>(entity)) {
					auto& camera = world.get_component_from_entity<CameraComponent>(entity);
					ImGui::Text("Camera Component:");
					ImGui::Text("  Type: %s", camera.m_Type == CameraComponent::CameraType::PERSPECTIVE ? "Perspective" : "Orthographic");
					ImGui::Text("  FOV: %.1f", camera.m_Fov);
					ImGui::Text("  Active: %s", camera.m_IsActive ? "Yes" : "No");
				}*/
				Render_Camera_Group_Component(entity);

				ImGui::Separator();

				// Action buttons
				if (ImGui::Button("Clear Selection")) {
					ClearEntitySelection();
				}

				break;
			}
		}

		if (!entityFound) {
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


void EditorMain::Render_Transform_Group_Component(ecs::entity entity_handle)
{

	glm::vec3 pos{};
	ecs::entity selected{};

	TransformComponent* transformComponent = nullptr;
	ScaleComponent* scaleComponent = nullptr;
	PositionComponent* positionComponent = nullptr;

	auto world = Engine::GetWorld();

	bool containTransform = world.has_all_components_in_entity<TransformComponent>(entity_handle);
	if (containTransform)
	{
		selected = entity_handle;


		transformComponent = &world.get_component_from_entity<TransformComponent>(selected);


	}







	/*for (auto& ent : scaleFilter)
	{
		if (static_cast<uint32_t>(ent.get_uid()) == m_SelectedEntityID)
		{
			if (world.has_all_components_in_entity<ScaleComponent>(ent))
			{
				scaleComponent = &world.get_component_from_entity<ScaleComponent>(ent);

			}

		}
	}*/


	if (transformComponent)
	{
		// Having a TransformComponent means we have a ScaleComponent and PositionComponent too
		if (ImGui::CollapsingHeader("Transform"))
		{
			PositionComponent& posComp = entity_handle.get<PositionComponent>();
			ScaleComponent& scaleComp = entity_handle.get<ScaleComponent>();

			glm::vec3 pos, scale;
			bool edited{};
			pos = posComp.m_WorldPos;
			scale = scaleComp.m_Scale;
			if (ImGui::InputFloat3("Position", &pos.x))
			{
				posComp.m_WorldPos = pos;
				edited = true;
			}

			if (ImGui::InputFloat3("Scale", &scale.x))
			{
				scaleComp.m_Scale = scale;
				edited = true;
			}

			if (edited)
			{
				transformComponent->m_trans = glm::translate(glm::mat4(1.0f), posComp.m_WorldPos) * glm::scale(glm::mat4(1.0f), scaleComp.m_Scale);
			}



		}
	}

}

void EditorMain::Render_Lighting_Group_Component(ecs::entity handle)
{


	auto world = Engine::GetWorld();
	ecs::entity selected{};

	LightComponent* lightComponent = nullptr;

	bool containLight = world.has_all_components_in_entity<LightComponent>(handle);
	if (!containLight) return;
	lightComponent = &world.get_component_from_entity<LightComponent>(handle);

	if (lightComponent)
	{
		if (ImGui::CollapsingHeader("Lighting"))
		{
			const char* lightTypes[] = { "Directional", "Point", "Spotlight" };
			int currentType = static_cast<int>(lightComponent->m_Type);
			if (ImGui::Combo("Light Type", &currentType, lightTypes, IM_ARRAYSIZE(lightTypes)))
			{
				lightComponent->m_Type = static_cast<Light::Type>(currentType);
			}
			ImGui::ColorEdit3("Color", &lightComponent->m_Color.r);
			ImGui::InputFloat("Intensity", &lightComponent->m_Intensity);


			ImGui::Checkbox("Enabled", &lightComponent->m_IsEnabled);

		}
	}
}


void EditorMain::Render_Camera_Group_Component(ecs::entity entity_handle)
{
	auto world = Engine::GetWorld();

	CameraComponent* cameraComponent = nullptr;

	if (world.has_all_components_in_entity<CameraComponent>(entity_handle))
	{
		cameraComponent = &world.get_component_from_entity<CameraComponent>(entity_handle);
	}

	if (cameraComponent)
	{
		float fov = cameraComponent->m_Fov;
		bool isActive = cameraComponent->m_IsActive;
		CameraComponent::CameraType type = cameraComponent->m_Type;

		if (ImGui::CollapsingHeader("Camera"))
		{
			const char* cameraTypes[] = { "Orthographic", "Perspective" };
			int currentType = static_cast<int>(type);
			if (ImGui::Combo("Camera Type", &currentType, cameraTypes, IM_ARRAYSIZE(cameraTypes)))
			{
				cameraComponent->m_Type = static_cast<CameraComponent::CameraType>(currentType);
			}
			if (cameraComponent->m_Type == CameraComponent::CameraType::PERSPECTIVE)
			{
				if (ImGui::InputFloat("Field of View", &fov))
				{
					cameraComponent->m_Fov = fov;
				}
			}
			else if (cameraComponent->m_Type == CameraComponent::CameraType::ORTHO)
			{
				// Orthographic-specific settings can go here
			}
			ImGui::Checkbox("Active", &isActive);
			cameraComponent->m_IsActive = isActive;
		}
	}
}

void EditorMain::Render_Mesh_Component(ecs::entity entity_handle)
{
	auto world = Engine::GetWorld();
	MeshRendererComponent* meshRendererComponent = nullptr;
	if (world.has_all_components_in_entity<MeshRendererComponent>(entity_handle))
	{
		meshRendererComponent = &world.get_component_from_entity<MeshRendererComponent>(entity_handle);
	}

	if (!meshRendererComponent) return;
	if (ImGui::CollapsingHeader("Mesh Renderer"))
	{
		ImGui::Text("  Mesh GUID: %s", meshRendererComponent->m_MeshGuid.to_hex().c_str());
		ImGui::Text("  Material GUID: %s", meshRendererComponent->m_MaterialGuid.to_hex().c_str());
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
		ImGui::MenuItem("Open Scene", "Ctrl+O");
		ImGui::MenuItem("Save Scene", "Ctrl+S");
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

	// Entity creation buttons
	if (ImGui::Button("Create Empty")) {
		CreateDefaultEntity();
	}
	ImGui::SameLine();
	if (ImGui::Button("Create Plane")) {
		CreatePlaneEntity();
	}
	ImGui::SameLine();
	if (ImGui::Button("Create Cube")) {
		CreateCube(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(1.0f), glm::vec3(0.5f, 0.5f, 1.0f));
	}
	ImGui::SameLine();
	if (ImGui::Button("Create Light")) {
		CreateLightEntity();
	}
	ImGui::SameLine();
	if (ImGui::Button("Create Camera")) {
		CreateCameraEntity();
	}

	ImGui::Separator();

	// List all entities in the world
	ecs::world world = Engine::GetWorld();

	// Get all entities with position component (basic entities)
	auto entities = world.filter_entities<PositionComponent>();

	ImGui::Text("Entities in scene:");
	for (auto entity : entities) {
		ImGui::PushID(static_cast<int>(entity.get_uid()));

		// Check if this entity is currently selected
		uint32_t entityUID = static_cast<uint32_t>(entity.get_uid());
		bool isSelected = (m_SelectedEntityID == entityUID);

		// Display entity info with selection highlighting
		std::string entityName = "Entity " + std::to_string(entity.get_uid());

		// Check what components this entity has
		bool hasTransform = world.has_all_components_in_entity<TransformComponent>(entity);
		bool hasMesh = world.has_all_components_in_entity<MeshRendererComponent>(entity);
		bool hasLight = world.has_all_components_in_entity<LightComponent>(entity);
		bool hasVisibility = world.has_all_components_in_entity<VisibilityComponent>(entity);

		if (hasLight) entityName += " (Light)";
		else if (hasMesh) entityName += " (Mesh)";
		else entityName += " (Empty)";

		// Highlight selected entity with different color
		if (isSelected) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow text for selected
		}

		if (ImGui::TreeNode(entityName.c_str())) {
			// Show component info
			if (world.has_all_components_in_entity<PositionComponent>(entity)) {
				auto pos = world.get_component_from_entity<PositionComponent>(entity);
				ImGui::Text("Position: %.2f, %.2f, %.2f", pos.m_WorldPos.x, pos.m_WorldPos.y, pos.m_WorldPos.z);
			}

			if (hasVisibility) {
				auto vis = world.get_component_from_entity<VisibilityComponent>(entity);
				ImGui::Text("Visible: %s", vis.m_IsVisible ? "Yes" : "No");
			}

			if (hasLight) {
				auto light = world.get_component_from_entity<LightComponent>(entity);
				ImGui::Text("Light Type: %d", static_cast<int>(light.m_Type));
				ImGui::Text("Intensity: %.2f", light.m_Intensity);
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
	// Example profiling data
	ImGui::Text("Frame Time: %.2fms", 16.67f);
	ImGui::Text("FPS: %.2f", Engine::Instance().GetInfo().m_FPS);

	auto events = Profiler::instance().getEventCurrentFrame();
	auto last = Profiler::instance().Get_Last_Frame();
	double frameMs = last.frameMs;

	ImGui::Text("Frame Time: %.2f ms", frameMs);
	ImGui::Text("FPS: %.2f", (frameMs > 0.0) ? 1000.0 / frameMs : 0.0);

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
		const char* modes[] = { "Fly", "Orbit", "Pan" };
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

void EditorMain::Render_Scene()
{
	// Update editor camera - will be done inside the Scene window with proper input handling
	if (!m_IsPlayMode && m_EditorCamera) {
		// Get delta time
		static auto lastTime = std::chrono::steady_clock::now();
		auto currentTime = std::chrono::steady_clock::now();
		float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
		lastTime = currentTime;

		// Camera update will be moved inside the viewport handling to avoid input conflicts
		// Store deltaTime for later use
		static float s_deltaTime = deltaTime;
		s_deltaTime = deltaTime;
	}

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
	auto& frameData = RenderSystem::Instance().m_SceneRenderer->GetFrameData();
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
			// Get stored delta time
			static auto lastTime = std::chrono::steady_clock::now();
			auto currentTime = std::chrono::steady_clock::now();
			float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
			lastTime = currentTime;

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
	auto allEntities = world.filter_entities<PositionComponent>();
	auto meshEntities = world.filter_entities<MeshRendererComponent>();
	auto lightEntities = world.filter_entities<LightComponent>();
	auto cameraEntities = world.filter_entities<CameraComponent>();

	int entityCount = 0, meshCount = 0, lightCount = 0, cameraCount = 0;
	for (auto entity : allEntities) {
		entityCount++;
		// Debug: Check what components each entity has
		bool hasMeshRenderer = world.has_all_components_in_entity<MeshRendererComponent>(entity);
		if (hasMeshRenderer) {
			//spdlog::info("DEBUG: Entity {} has MeshRendererComponent", entity.get_uid());
		}
	}
	for (auto entity : meshEntities) { meshCount++; }
	for (auto entity : lightEntities) { lightCount++; }
	for (auto entity : cameraEntities) { cameraCount++; }

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
	world.add_component_to_entity<PositionComponent>(entity, glm::vec3(0.0f, 0.0f, 0.0f));
	world.add_component_to_entity<TransformComponent>(entity, glm::mat4(1.0f));
	world.add_component_to_entity<VisibilityComponent>(entity, true);
}

void EditorMain::CreatePlaneEntity()
{
	ecs::world world = Engine::GetWorld();

	// Create new entity
	auto entity = world.add_entity();

	// Add transform components
	world.add_component_to_entity<PositionComponent>(entity, glm::vec3(0.0f, 0.0f, 0.0f));
	world.add_component_to_entity<TransformComponent>(entity, glm::mat4(1.0f));
	world.add_component_to_entity<VisibilityComponent>(entity, true);

	// Get shared plane mesh from RenderSystem
	auto planeMesh = RenderSystem::GetSharedPlaneMesh();
	assert(planeMesh && "Shared plane mesh must be available");

	// Get PBR shader from RenderSystem
	auto shader = RenderSystem::s_CubeShader;
	if (!shader) {
		RenderSystem::LoadBasicShaders();
		shader = RenderSystem::s_CubeShader;
	}
	assert(shader && "PBR shader must be available");

	// Create material with PBR shader
	auto material = std::make_shared<Material>(shader, "PlaneMaterial_" + std::to_string(entity.get_uid()));
	material->SetAlbedoColor(glm::vec3(0.8f, 0.3f, 0.3f)); // Red color
	material->SetMetallicValue(0.1f);
	material->SetRoughnessValue(0.8f);

	assert(material && "Material creation failed");

	// Generate GUIDs for the resources
	auto meshGuid = Resource::Guid::generate();
	auto materialGuid = Resource::Guid::generate();

	// Register resources with the RenderSystem's editor cache
	RenderSystem::RegisterEditorMesh(meshGuid, planeMesh);
	RenderSystem::RegisterEditorMaterial(materialGuid, material);

	// Add mesh renderer component
	MeshRendererComponent meshRenderer;
	meshRenderer.m_MeshGuid = meshGuid;
	meshRenderer.m_MaterialGuid = materialGuid;

	world.add_component_to_entity<MeshRendererComponent>(entity, meshRenderer);
}

void EditorMain::CreateLightEntity()
{
	ecs::world world = Engine::GetWorld();

	// Create new entity
	auto entity = world.add_entity();

	// Add position
	world.add_component_to_entity<PositionComponent>(entity, glm::vec3(0.0f, 5.0f, 0.0f));

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
	world.add_component_to_entity<PositionComponent>(entity, glm::vec3(0.0f, 2.0f, 5.0f));

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

void EditorMain::CreateDemoScene()
{
	spdlog::info("Creating demo scene with cubes and lighting");

	// Create 3x3 cube grid using local utilities
	CreateCubeGrid(3, 3.0f);

	// Create additional directional light for better lighting
	ecs::world world = Engine::GetWorld();

	auto lightEntity = world.add_entity();
	world.add_component_to_entity<PositionComponent>(lightEntity, glm::vec3(0.0f, 5.0f, 0.0f));

	LightComponent light;
	light.m_Type = Light::Type::Directional;
	light.m_Direction = glm::vec3(0.2f, -0.8f, 0.3f);
	light.m_Color = glm::vec3(1.0f, 0.95f, 0.85f);
	light.m_Intensity = 1.0f;
	light.m_IsEnabled = true;

	world.add_component_to_entity<LightComponent>(lightEntity, light);

	// Set stronger ambient light for better visibility
	RenderSystem::Instance().m_SceneRenderer->SetAmbientLight(glm::vec3(0.3f, 0.3f, 0.3f));

	spdlog::info("Demo scene created with 9 cubes and enhanced lighting");
}

void EditorMain::CreateCube(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& color)
{
	assert(scale.x > 0 && scale.y > 0 && scale.z > 0 && "Scale must be positive");

	auto world = Engine::GetWorld();
	auto entity = world.add_entity();

	// Add transform components
	world.add_component_to_entity<PositionComponent>(entity, position);
	world.add_component_to_entity<ScaleComponent>(entity, scale);
	world.add_component_to_entity<TransformComponent>(entity,
		glm::scale(glm::translate(glm::mat4(1.0f), position), scale));

	world.add_component_to_entity<VisibilityComponent>(entity, true);

	// Get shared cube mesh from RenderSystem (enables proper instancing)
	auto cubeMesh = RenderSystem::GetSharedCubeMesh();
	assert(cubeMesh && "Shared cube mesh must be available");

	// Get PBR shader from RenderSystem
	auto shader = RenderSystem::s_CubeShader;
	if (!shader) {
		RenderSystem::LoadBasicShaders();
		shader = RenderSystem::s_CubeShader;
	}
	assert(shader && "PBR shader must be available");

	// Create PBR material with enhanced properties for better visibility
	auto material = std::make_shared<Material>(shader, "EditorCube_" + std::to_string(entity.get_uid()));
	material->SetAlbedoColor(color * 1.2f); // Brighten the color slightly
	material->SetMetallicValue(0.0f);       // Non-metallic for better color visibility
	material->SetRoughnessValue(0.6f);      // Medium roughness

	assert(material && "Material creation failed");

	// Register in editor cache (use shared mesh for all cubes!)
	auto meshGuid = Resource::Guid::generate();
	auto materialGuid = Resource::Guid::generate();

	RenderSystem::RegisterEditorMesh(meshGuid, cubeMesh);
	RenderSystem::RegisterEditorMaterial(materialGuid, material);

	// Add mesh renderer component
	MeshRendererComponent meshRenderer;
	meshRenderer.m_MeshGuid = meshGuid;
	meshRenderer.m_MaterialGuid = materialGuid;

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
	auto* sceneRenderer = RenderSystem::Instance().m_SceneRenderer.get();
	assert(sceneRenderer && "SceneRenderer must be available for entity picking");

	// Assert input parameters are valid
	assert(viewportWidth > 0.0f && viewportHeight > 0.0f && "Viewport dimensions must be positive");
	assert(mouseX >= 0.0f && mouseY >= 0.0f && "Mouse coordinates must be non-negative");
	assert(mouseX < viewportWidth && mouseY < viewportHeight && "Mouse coordinates must be within viewport");

	spdlog::info("Editor: Performing entity picking at viewport position ({:.1f}, {:.1f}) in viewport ({:.1f}x{:.1f})",
		mouseX, mouseY, viewportWidth, viewportHeight);

	// Debug: Check how many entities exist and have renderable components
	ecs::world world = Engine::GetWorld();
	auto allEntities = world.filter_entities<PositionComponent>();
	auto renderableEntities = world.filter_entities<MeshRendererComponent, VisibilityComponent>();

	int totalEntities = 0;
	int renderableCount = 0;
	for (auto entity : allEntities) { totalEntities++; }
	for (auto entity : renderableEntities) { renderableCount++; }

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

void EditorMain::HandleViewportPicking()
{
	// This function can be called to handle other picking-related logic
	// For now, picking is handled directly in Render_Scene()
}