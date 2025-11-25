#include "Screens/EditorMain.hpp"

#include "Profiler/profiler.hpp"
#include "Bindings/MANAGED_CONSOLE.hpp"
#include "Scene/Scene.hpp"
#include "Render/Render.h"
#include "Physics/Physics_System.h"
#include "GLFW/glfw3.h"

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
