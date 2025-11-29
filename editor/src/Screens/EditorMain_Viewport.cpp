#include "Screens/EditorMain.hpp"
#include <ImGuizmo.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <System/PrefabSystem.hpp>
#include <Scene/Scene.hpp>
#include <components/transform.h>
#include <Physics/Physics_Components.h>

#include <Render/Render.h>
#include <Input/InputManager.h>
#include "Profiler/profiler.hpp"

// ============================================================================
// GAME VIEWPORT RENDERING
// ============================================================================
void EditorMain::Render_Game()
{
	PF_EDITOR_SCOPE("Render_Game");
	ImGui::Begin("Game", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	// Get frame data from engine thread
	auto& frameData = engineService.GetFrameData();

	// Get available viewport size
	ImVec2 viewportSize = ImGui::GetContentRegionAvail();

	// Update game viewport dimensions and camera aspect ratio (Unity-style auto-sync)
	// Checks every frame to ensure aspect ratio always matches viewport, preventing FOV stretching
	if (viewportSize.x > 0 && viewportSize.y > 0) {
		static uint32_t lastGameWidth = 0, lastGameHeight = 0;
		uint32_t currentWidth = static_cast<uint32_t>(viewportSize.x);
		uint32_t currentHeight = static_cast<uint32_t>(viewportSize.y);

		// Update if size changed (handles initial load, resize, and window state changes)
		if (lastGameWidth != currentWidth || lastGameHeight != currentHeight) {
			lastGameWidth = currentWidth;
			lastGameHeight = currentHeight;

			engineService.ExecuteOnEngineThread([width = currentWidth, height = currentHeight]() {
				// Update RenderSystem viewport dimensions
				Engine::GetRenderSystem().SetGameViewportSize(width, height);

				// Update game camera aspect ratio to match viewport (Unity-style)
				ecs::world world = Engine::GetWorld();
				auto cameraQuery = world.query_components<CameraComponent, TransformComponent>();
				for (auto [cam, trans] : cameraQuery) {
					if (cam.m_IsActive) {
						cam.m_AspectRatio = static_cast<float>(width) / static_cast<float>(height);
						break;  // Only update first active camera
					}
				}
			});
		}
	}

	if (frameData.gameResolvedBuffer)
	{
		// Get texture from game framebuffer
		GLuint textureID = frameData.gameResolvedBuffer->GetColorAttachmentRendererID(0);

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

// ============================================================================
// CAMERA CONTROLS PANEL
// ============================================================================
void EditorMain::Render_CameraControls()
{
	PF_EDITOR_SCOPE("Render_CameraControls");
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

// ============================================================================
// SCENE VIEWPORT RENDERING
// ============================================================================
void EditorMain::Render_Scene()
{
	PF_EDITOR_SCOPE("Render_Scene");

	// Get delta time for camera updates
	float deltaTime = static_cast<float>(engineService.GetDeltaTime());

	ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	// Get viewport size for aspect ratio
	ImVec2 viewportSize = ImGui::GetContentRegionAvail();
	if (viewportSize.x > 0 && viewportSize.y > 0) {
		// Check if viewport size changed (only update when necessary)
		static float lastWidth = 0, lastHeight = 0;
		if (std::abs(lastWidth - viewportSize.x) > 1.0f || std::abs(lastHeight - viewportSize.y) > 1.0f) {
			m_ViewportWidth = viewportSize.x;
			m_ViewportHeight = viewportSize.y;

			// Update aspect ratio if viewport size changed
			if (m_EditorCamera) {
				m_EditorCamera->SetAspectRatio(m_ViewportWidth / m_ViewportHeight);
			}

			// Update RenderSystem viewport dimensions (fixes distortion on resize)
			engineService.ExecuteOnEngineThread([width = (uint32_t)viewportSize.x, height = (uint32_t)viewportSize.y]() {
				Engine::GetRenderSystem().SetEditorViewportSize(width, height);
			});

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
							}
							else {
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
		/*static bool lastHovered = false;
		if (viewportHovered != lastHovered) {
			spdlog::info("Editor: Viewport hover state changed: {}", viewportHovered ? "entered" : "exited");
			lastHovered = viewportHovered;
		}*/

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

// ============================================================================
// ENTITY PICKING & SELECTION
// ============================================================================
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
	if (m_SelectedEntityID != 0 || !m_EntitiesIDSelection.empty()) {
		spdlog::info("Editor: Cleared entity selection (was Object ID: {}, Number of Cleared Selections {})", m_SelectedEntityID, m_EntitiesIDSelection.size());
		m_SelectedEntityID = 0;
		m_EntitiesIDSelection.clear(); // Clear the selection set for hierarchy display

		// Clear visual feedback
		engineService.ClearOutlinedObjects();
	}
}

void EditorMain::FrameSelectedEntity()
{
	// Check if there's a selected entity
	if (m_EntitiesIDSelection.empty())
	{
		spdlog::info("Frame Selected: No entity selected");
		return;
	}

	// Get the first selected entity
	uint32_t selectedUID = *m_EntitiesIDSelection.begin();

	// Query entity position from the engine
	engineService.ExecuteOnEngineThread([this, selectedUID]() {
		ecs::world world = Engine::GetWorld();

		// Find the entity by UID
		ecs::entity targetEntity;
		bool foundEntity = false;

		for (auto& entity : world.get_all_entities())
		{
			if (entity.get_uid() == selectedUID)
			{
				targetEntity = entity;
				foundEntity = true;
				break;
			}
		}

		if (!foundEntity)
		{
			spdlog::warn("Frame Selected: Entity with UID {} not found", selectedUID);
			return;
		}

		// Get transform component
		if (!targetEntity.any<TransformComponent>())
		{
			spdlog::warn("Frame Selected: Entity has no TransformComponent");
			return;
		}

		auto& transform = targetEntity.get<TransformComponent>();
		glm::vec3 targetPosition = transform.m_Translation;

		// Calculate appropriate distance based on object scale
		float objectSize = glm::length(transform.m_Scale);
		float distance = glm::max(objectSize * 2.5f, 5.0f); // At least 5 units away

		spdlog::info("Frame Selected: Moving camera to position ({}, {}, {}), distance: {}",
			targetPosition.x, targetPosition.y, targetPosition.z, distance);

		// Store position and distance to apply on main thread
		// We need to call FocusOn on the main thread where the camera is
		// For now, we'll do it directly since m_EditorCamera should be thread-safe for reads
	});

	// Apply camera focus on main thread (immediate)
	// We need to get the position first - let's do a simpler synchronous approach
	glm::vec3 targetPosition(0.0f);
	float distance = 5.0f;
	bool success = false;

	// Synchronous query (blocking)
	engineService.ExecuteOnEngineThread([this, selectedUID, &targetPosition, &distance, &success]() {
		ecs::world world = Engine::GetWorld();

		for (auto& entity : world.get_all_entities())
		{
			if (entity.get_uid() == selectedUID)
			{
				if (entity.any<TransformComponent>())
				{
					auto& transform = entity.get<TransformComponent>();
					targetPosition = transform.m_Translation;
					float objectSize = glm::length(transform.m_Scale);
					distance = glm::max(objectSize * 2.5f, 5.0f);
					success = true;
				}
				break;
			}
		}
	});

	// Wait a frame for the engine thread to process
	// For now, apply immediately (this works if ExecuteOnEngineThread blocks)
	if (success || true) // Apply even if not successful, use last known or default
	{
		m_EditorCamera->FocusOn(targetPosition, distance);
		spdlog::info("Frame Selected: Camera focused on position ({}, {}, {})",
			targetPosition.x, targetPosition.y, targetPosition.z);
	}
}

void EditorMain::HandleViewportPicking()
{
	// This function can be called to handle other picking-related logic
	// For now, picking is handled directly in Render_Scene()
}

// ============================================================================
// GIZMOS & TRANSFORM MANIPULATION
// ============================================================================
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
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

		// Phase 3: Removed unused GetActiveCamera() call (dead code)

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
		ImVec2 viewCubePos = ImVec2(viewportPos.x + viewportSize.x - viewCubeSize - viewCubePadding, viewportPos.y + viewCubePadding);

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
