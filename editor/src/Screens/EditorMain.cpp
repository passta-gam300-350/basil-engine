// THIS IS THE ACTUAL LEVEL EDITOR!!!, 

#include <Render/Render.h>
#include <Engine.hpp>
#include <components/transform.h>
#include <Resources/PrimitiveGenerator.h>
#include <Resources/Material.h>
#include <Input/InputManager.h>
#include <unordered_map>
#include <spdlog/spdlog.h>

#include "Screens/EditorMain.hpp"

#include "Editor.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "GLFW/glfw3.h"

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

	// Create a basic light so we can see something
	CreateLightEntity();
	CreateCameraEntity();

	//std::jthread jth(&Engine::Update);
}

void EditorMain::render()
{
	// Menu Bar
	if (!active) return;

	Render_MenuBar();


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


	ImGui::Begin("Inspector", nullptr);
	ImGui::End();
	glClearColor(1, 1, 1, 1);

	// Process camera input on ECS camera entity
	ProcessCameraInput();

	// Update engine systems
	ecs::world world = Engine::GetWorld();
	RenderSystem::System().Update(world);

	Render_SceneExplorer();
	Render_Console();
	Render_Scene();
	Render_Game();

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

		// Display entity info
		std::string entityName = "Entity " + std::to_string(entity.get_uid());

		// Check what components this entity has
		bool hasTransform = world.has_all_components_in_entity<TransformComponent>(entity);
		bool hasMesh = world.has_all_components_in_entity<MeshRendererComponent>(entity);
		bool hasLight = world.has_all_components_in_entity<LightComponent>(entity);
		bool hasVisibility = world.has_all_components_in_entity<VisibilityComponent>(entity);

		if (hasLight) entityName += " (Light)";
		else if (hasMesh) entityName += " (Mesh)";
		else entityName += " (Empty)";

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

		ImGui::PopID();
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

void EditorMain::Render_Scene()
{
	ImGui::Begin("Scene");

	// Check if editor framebuffer is available before trying to access it
	auto& frameData = RenderSystem::Instance().m_SceneRenderer->GetFrameData();
	if (frameData.editorColorBuffer && frameData.editorColorBuffer->GetFBOHandle() != 0) {
		ImGui::Image((ImTextureID)frameData.editorColorBuffer->GetFBOHandle(),
		             {1280,720}, ImVec2(0, 1), ImVec2(1, 0));
	} else {
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
			spdlog::info("DEBUG: Entity {} has MeshRendererComponent", entity.get_uid());
		}
	}
	for (auto entity : meshEntities) { meshCount++; }
	for (auto entity : lightEntities) { lightCount++; }
	for (auto entity : cameraEntities) { cameraCount++; }

	ImGui::Text("Total entities: %d", entityCount);
	ImGui::Text("Mesh entities: %d", meshCount);
	ImGui::Text("Light entities: %d", lightCount);
	ImGui::Text("Camera entities: %d", cameraCount);

	// Show framebuffer status
	//auto& frameData = RenderSystem::Instance().m_SceneRenderer->GetFrameData();
	ImGui::Text("Editor FBO: %s", frameData.editorColorBuffer ? "Valid" : "None");

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

	// Create a simple plane using graphics library primitives
	auto planeMesh = PrimitiveGenerator::CreatePlane(2.0f, 2.0f, 1, 1);

	// Get the SceneRenderer to access ResourceManager
	auto& renderSystem = RenderSystem::Instance();
	auto* resourceManager = renderSystem.m_SceneRenderer->GetResourceManager();

	// Try to create a basic shader
	auto shader = resourceManager->LoadShader("test", "basic.vert", "basic.frag");

	if (shader) {
		// Create material with the shader
		auto material = std::make_shared<Material>(shader, "PlaneMaterial");
		material->SetAlbedoColor(glm::vec3(0.8f, 0.3f, 0.3f)); // Red color

		// Create shared pointers for mesh and material
		auto meshPtr = std::make_shared<Mesh>(std::move(planeMesh));

		// Generate GUIDs for the resources
		auto meshGuid = Resource::Guid::generate();
		auto materialGuid = Resource::Guid::generate();

		// Register resources with the RenderSystem's editor cache (proper way)
		RenderSystem::RegisterEditorMesh(meshGuid, meshPtr);
		RenderSystem::RegisterEditorMaterial(materialGuid, material);

		// Add mesh renderer component
		MeshRendererComponent meshRenderer;
		meshRenderer.m_MeshGuid = meshGuid;
		meshRenderer.m_MaterialGuid = materialGuid;

		world.add_component_to_entity<MeshRendererComponent>(entity, meshRenderer);

		// Debug: Confirm mesh component was added
		spdlog::info("DEBUG: Added MeshRendererComponent to entity {}", entity.get_uid());
	} else {
		// Debug: Shader loading failed
		spdlog::warn("DEBUG: Failed to load shader for cube entity");
	}
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

void EditorMain::ProcessCameraInput()
{
	ecs::world world = Engine::GetWorld();
	auto cameras = world.filter_entities<CameraComponent, PositionComponent>();

	if (!cameras) return;

	// Get the first camera entity
	auto [cameraComp, cameraPos] = (*cameras.begin()).get<CameraComponent, PositionComponent>();

	// Try InputManager first (if available), fallback to direct GLFW
	auto* inputManager = InputManager::Get_Instance();

	// Check for camera control toggle (F1 key like GraphicsTestDriver)
	static bool f1WasPressed = false;
	bool f1IsPressed = (inputManager && inputManager->Is_KeyPressed(GLFW_KEY_F1)) ||
	                   (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS);

	if (f1IsPressed && !f1WasPressed) {
		m_CameraControlEnabled = !m_CameraControlEnabled;
		if (m_CameraControlEnabled) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			m_FirstMouse = true;
		} else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
	f1WasPressed = f1IsPressed;

	if (!m_CameraControlEnabled) return;

	// Mouse look (when camera control is enabled)
	double mouseX, mouseY;
	if (inputManager) {
		inputManager->Get_MousePosition(mouseX, mouseY);
	} else {
		glfwGetCursorPos(window, &mouseX, &mouseY);
	}

	if (m_FirstMouse) {
		m_LastMouseX = static_cast<float>(mouseX);
		m_LastMouseY = static_cast<float>(mouseY);
		m_FirstMouse = false;
	}

	float xoffset = static_cast<float>(mouseX) - m_LastMouseX;
	float yoffset = m_LastMouseY - static_cast<float>(mouseY); // Reversed since y-coordinates go from bottom to top
	m_LastMouseX = static_cast<float>(mouseX);
	m_LastMouseY = static_cast<float>(mouseY);

	xoffset *= m_MouseSensitivity;
	yoffset *= m_MouseSensitivity;

	cameraComp.m_Yaw += xoffset;
	cameraComp.m_Pitch += yoffset;

	// Constrain pitch to prevent camera flipping
	if (cameraComp.m_Pitch > 89.0f) cameraComp.m_Pitch = 89.0f;
	if (cameraComp.m_Pitch < -89.0f) cameraComp.m_Pitch = -89.0f;

	// Update camera vectors based on yaw and pitch
	glm::vec3 front;
	front.x = cos(glm::radians(cameraComp.m_Yaw)) * cos(glm::radians(cameraComp.m_Pitch));
	front.y = sin(glm::radians(cameraComp.m_Pitch));
	front.z = sin(glm::radians(cameraComp.m_Yaw)) * cos(glm::radians(cameraComp.m_Pitch));
	cameraComp.m_Front = glm::normalize(front);

	// Recalculate Right and Up vectors
	glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	cameraComp.m_Right = glm::normalize(glm::cross(cameraComp.m_Front, worldUp));
	cameraComp.m_Up = glm::normalize(glm::cross(cameraComp.m_Right, cameraComp.m_Front));

	// Camera movement parameters (matching GraphicsTestDriver)
	float deltaTime = 0.016f; // ~60fps
	float moveSpeed = 2.5f;   // Movement speed similar to GraphicsTestDriver

	glm::vec3 position = cameraPos.m_WorldPos;

	// Apply movement based on input using updated camera vectors
	if ((inputManager && inputManager->Is_KeyPressed(GLFW_KEY_W)) ||
	    glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		position += cameraComp.m_Front * moveSpeed * deltaTime;
	}
	if ((inputManager && inputManager->Is_KeyPressed(GLFW_KEY_S)) ||
	    glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		position -= cameraComp.m_Front * moveSpeed * deltaTime;
	}
	if ((inputManager && inputManager->Is_KeyPressed(GLFW_KEY_A)) ||
	    glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		position -= cameraComp.m_Right * moveSpeed * deltaTime;
	}
	if ((inputManager && inputManager->Is_KeyPressed(GLFW_KEY_D)) ||
	    glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		position += cameraComp.m_Right * moveSpeed * deltaTime;
	}
	if ((inputManager && inputManager->Is_KeyPressed(GLFW_KEY_SPACE)) ||
	    glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		position += cameraComp.m_Up * moveSpeed * deltaTime;
	}
	if ((inputManager && inputManager->Is_KeyPressed(GLFW_KEY_LEFT_CONTROL)) ||
	    glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		position -= cameraComp.m_Up * moveSpeed * deltaTime;
	}

	// Update the camera entity position - RenderSystem will read this and pass to graphics lib
	cameraPos.m_WorldPos = position;
}


