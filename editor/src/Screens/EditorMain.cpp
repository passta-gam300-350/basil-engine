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
#include <filesystem>

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
	
	m_AssetManager = std::make_unique<AssetManager>(Editor::GetInstance().GetConfig().project_workingDir + "/assets", Editor::GetInstance().GetConfig().project_workingDir + "/.imports");
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

	// Create demo scene with cubes for testing (disabled by default)
	// CreateDemoScene();

	//std::jthread jth(&Engine::Update);
}

void EditorMain::render()
{
	// Menu Bar
	if (!active) return;

	// Update input manager ONCE per frame - at the very beginning
	auto* input = InputManager::Get_Instance();
	input->Update();

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

	Render_SceneExplorer();
	Render_Console();
	Render_Scene();
	Render_Game();
	Render_CameraControls();
	Render_AssetBrowser();
}

void EditorMain::cleanup()
{
	m_AssetManager.reset(nullptr);
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
	} else {
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
		ImGui::PushID(filename.c_str());
		ImGui::Button(filename.c_str(), {thumbnailSize, thumbnailSize}); // Creates a button for the folders
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
			size_t pos = 0;
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
	
	ImGui::Begin("Assets");
	ImGui::Columns(columns, 0, false);
	//this map is single threaded
	for (auto[assetname, guid] : m_AssetManager->m_AssetNameGuid) {
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
	// Update editor camera OUTSIDE of ImGui window - do this first
	if (!m_IsPlayMode && m_EditorCamera) {
		// Get delta time
		static auto lastTime = std::chrono::steady_clock::now();
		auto currentTime = std::chrono::steady_clock::now();
		float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
		lastTime = currentTime;

		// Temporarily disable ImGui input capture to allow camera input
		// We'll check viewport hover inside the Scene window
		ImGuiIO& io = ImGui::GetIO();
		bool originalWantCaptureMouse = io.WantCaptureMouse;
		bool originalWantCaptureKeyboard = io.WantCaptureKeyboard;

		// Always allow camera input - we'll handle viewport detection differently
		io.WantCaptureMouse = false;
		io.WantCaptureKeyboard = false;

		// Update camera based on input
		m_EditorCamera->Update(deltaTime);

		// Restore original ImGui input capture state
		io.WantCaptureMouse = originalWantCaptureMouse;
		io.WantCaptureKeyboard = originalWantCaptureKeyboard;


		// Handle scroll for zoom (basic implementation)
		double scrollX, scrollY;
		auto* input = InputManager::Get_Instance();
		input->Get_ScrollOffset(scrollX, scrollY);
		if (scrollY != 0) {
			m_EditorCamera->OnMouseScroll(static_cast<float>(scrollY));
		}

		// Camera matrices are now updated in main render() after RenderSystem::Update
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
	}

	// Check if editor framebuffer is available before trying to access it
	auto& frameData = RenderSystem::Instance().m_SceneRenderer->GetFrameData();
	if (frameData.editorColorBuffer && frameData.editorColorBuffer->GetFBOHandle() != 0) {
		ImGui::Image((ImTextureID)frameData.editorColorBuffer->GetFBOHandle(),
		             viewportSize, ImVec2(0, 1), ImVec2(1, 0));
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

	spdlog::info("Demo scene created with 9 cubes and enhanced lighting");
}

void EditorMain::CreateCube(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& color)
{
	assert(scale.x > 0 && scale.y > 0 && scale.z > 0 && "Scale must be positive");

	auto world = Engine::GetWorld();
	auto entity = world.add_entity();

	// Add transform components
	world.add_component_to_entity<PositionComponent>(entity, position);
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

	// Create PBR material
	auto material = std::make_shared<Material>(shader, "EditorCube_" + std::to_string(entity.get_uid()));
	material->SetAlbedoColor(color);
	material->SetMetallicValue(0.1f);
	material->SetRoughnessValue(0.8f);

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