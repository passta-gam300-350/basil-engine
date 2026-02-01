#include "Screens/EditorMain.hpp"
#include "System/PrefabSystem.hpp"
#include "System/Audio.hpp"

#include <Scene/Scene.hpp>
#include <Component/Behaviour.hpp>
#include <Component/MaterialOverridesComponent.hpp>
#include <Component/Transform.hpp>
#include <components/behaviour.hpp>
#include <Render/Render.h>
#include "Physics/Physics_Components.h"
#include "Profiler/profiler.hpp"

template <typename T>
void assign_enum_helper(void* dest, const void* src) {
	*reinterpret_cast<T*>(dest) = *reinterpret_cast<const T*>(src);
};

void EditorMain::Render_Inspector()
{
	PF_EDITOR_SCOPE("Render_Inspector");

	// OPTIMIZATION: Check if window is visible before doing expensive work
	if (!ImGui::Begin("Inspector", &showInspector)) {
		ImGui::End();
		return;
	}

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
	static const ReflectionRegistry::TypeID skip_relationship_component{ std::find_if(ReflectionRegistry::types().begin(), ReflectionRegistry::types().end(), [](auto const& kv) {return kv.second.id() == ToTypeName("Relationship"); })->first };
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

	ReflectionRegistry::TypeID hud_component{};
	auto hudIt = internal_type_map.find(entt::type_index<HUDComponent>::value());
	if (hudIt != internal_type_map.end())
	{
		hud_component = hudIt->second;
	}

	for (auto const& [type_id, uptr] : component_list) {
		if (type_id == skip_name_component || type_id == skip_sceneid_component || type_id == skip_relationship_component || type_id == prefab_component_id) {
			continue;  // Skip PrefabComponent as it's internal
		}
		if (behaviour_component && type_id == behaviour_component)
		{
			if (behaviour* behaviour_component_ptr = reinterpret_cast<behaviour*>(uptr.get()))
			{
				// Unity-style: Behaviour component header is brighter than background, even brighter on hover
				// Background is 0.196, header should be noticeably brighter at ~0.26
				ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.26f, 0.26f, 0.26f, 1.0f));  // Brighter than background (0.196)
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.32f, 0.32f, 0.32f, 1.0f));  // Even brighter on hover
				ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.36f, 0.36f, 0.36f, 1.0f));  // Brightest when clicked

				// Make header text slightly larger for better clarity (Unity-like)
				ImGui::SetWindowFontScale(1.1f);

				// Add Framed flag to make header background visible
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;
				if (ImGui::TreeNodeEx("Behaviour", flags))
				{
					// Reset font scale after header
					ImGui::SetWindowFontScale(1.0f);
					ImGui::PopStyleColor(3);  // Pop header colors after TreeNode
					Render_Behaviour_Component(*behaviour_component_ptr);
					ImGui::TreePop();
				}
				else
				{
					// Reset font scale and pop colors if not opened
					ImGui::SetWindowFontScale(1.0f);
					ImGui::PopStyleColor(3);  // Pop even if not opened
				}

				// Unity-style: Add separator between components
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
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

		// Unity-style: Component headers are brighter than background, even brighter on hover
		// Apply colored background for components with overrides
		int colorsPushed = 0;
		if (hasOverrides)
		{
			// Blue tint for overridden components (like Unity's prefab overrides)
			// Background is 0.196, header should be brighter
			
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.22f, 0.38f, 0.54f, 1.0f));  // Brighter blue header
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.28f, 0.46f, 0.64f, 1.0f));  // Even brighter on hover
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.32f, 0.52f, 0.72f, 1.0f));  // Brightest when clicked
			colorsPushed = 3;
		}
		else
		{
			// Normal components: slightly brighter gray header (Unity-like)
			// Background is 0.196, header should be noticeably brighter at ~0.26
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.26f, 0.26f, 0.26f, 1.0f));  // Brighter than background (0.196)
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.32f, 0.32f, 0.32f, 1.0f));  // Even brighter on hover
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.36f, 0.36f, 0.36f, 1.0f));  // Brightest when clicked
			colorsPushed = 3;
		}

		// Component header with override count
		std::string headerLabel = std::string(componentLabel);
		if (hasOverrides)
		{
			headerLabel += " (" + std::to_string(overrideCount) + " overrides)";
		}

		// Make header text slightly larger for better clarity (Unity-like)
		ImGui::SetWindowFontScale(1.1f);

		// Add Framed flag to make header background visible
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (ImGui::TreeNodeEx(headerLabel.c_str(), flags)) {
			// Reset font scale after header
			ImGui::SetWindowFontScale(1.0f);
			// Pop header colors immediately after TreeNode (content area uses default background)
			ImGui::PopStyleColor(colorsPushed);

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

				// Unity-style: Add separator between components
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
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

				// Unity-style: Add separator between components
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
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

				// Unity-style: Add separator between components
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
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

				// Unity-style: Add separator between components
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
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
								}
								else if (audio.isPaused) {
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
							}
							else {
								spdlog::warn("Entity must have both MeshRendererComponent and MaterialOverridesComponent to populate overrides");
							}
							});
						ul.lock();
					}
					ImGui::SameLine();
					ImGui::TextDisabled("Adds standard PBR properties (u_MetallicValue, u_RoughnessValue, u_AlbedoColor)");
				}
			}

			// Special UI section for HUDComponent
			if (hud_component && type_id == hud_component) {
				if (HUDComponent* hudComp = reinterpret_cast<HUDComponent*>(uptr.get())) {
					ImGui::Separator();

					// Only show button if texture is assigned
					bool hasTexture = (hudComp->m_TextureGuid.m_guid != rp::null_guid);

					if (!hasTexture) {
						ImGui::BeginDisabled();
					}

					if (ImGui::Button("Set Native Size", ImVec2(-1, 0))) {
						ul.unlock();
						engineService.ExecuteOnEngineThread([entityHandle = engineService.m_cont->m_snapshot_entity_handle]() {
							ecs::entity entity{ entityHandle };
							if (entity.all<HUDComponent>()) {
								HUDComponent& hud = entity.get<HUDComponent>();

								// Load texture and get dimensions
								auto& registry = ResourceRegistry::Instance();
								auto* texturePtr = registry.Get<std::shared_ptr<Texture>>(hud.m_TextureGuid.m_guid);

								if (texturePtr && *texturePtr) {
									unsigned int textureID = (*texturePtr)->id;

									// Query texture dimensions from OpenGL
									glBindTexture(GL_TEXTURE_2D, textureID);
									int width = 0, height = 0;
									glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
									glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
									glBindTexture(GL_TEXTURE_2D, 0);

									if (width > 0 && height > 0) {
										hud.size = glm::vec2(static_cast<float>(width), static_cast<float>(height));
										spdlog::info("Set HUD native size to {}x{}", width, height);
									}
									else {
										spdlog::warn("Failed to get texture dimensions for HUD component");
									}
								}
							}
							});
						ul.lock();
					}

					if (!hasTexture) {
						ImGui::EndDisabled();
					}

					if (!hasTexture && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
						ImGui::SetTooltip("Assign a texture to use Set Native Size");
					}
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
		else
		{
			// If tree node was not opened, reset font scale and pop color styling
			ImGui::SetWindowFontScale(1.0f);
			ImGui::PopStyleColor(colorsPushed);
		}

		// Unity-style: Add separator between components for clear visual distinction
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
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
		else if (glm::vec4* vec4_ptr = value.try_cast<glm::vec4>()) {
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
				void(*assignment_by_type)(void*, const void*) { nullptr };

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
				if (ImGui::DragInt(field_name.c_str(), vi)) {
					is_dirty = true;
				}
			}
			else if (float* vf = value.try_cast<float>()) {
				if (ImGui::DragFloat(field_name.c_str(), vf)) {
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
				if (field_name.find("layer") != std::string::npos) {
					uint8_t maxLayer = 10;
					if (ImGui::SliderScalar(field_name.c_str(), ImGuiDataType_U8, vui8, &minValue, &maxLayer, "%u")) {
						is_dirty = true;
					}
				}
				else {
					if (ImGui::SliderScalar(field_name.c_str(), ImGuiDataType_U8, vui8, &minValue, &maxValue, "%u")) {
						is_dirty = true;
					}
				}
			}
			else if (uint32_t* vui = value.try_cast<uint32_t>()) {
				uint32_t minValue = 0;
				uint32_t maxValue = 2147483647; // UINT32_MAX

				// With custom format
				if (ImGui::SliderScalar(field_name.c_str(), ImGuiDataType_U32, vui, &minValue, &maxValue, "%u")) {
					is_dirty = true;
				}

			}
			else if (std::string* vs = value.try_cast<std::string>()) {
				// Use a buffer for ImGui::InputText, then copy back to string
				char buffer[1024];
				strncpy_s(buffer, vs->c_str(), sizeof(buffer) - 1);
				buffer[sizeof(buffer) - 1] = '\0';

				ImGui::Text("%s", field_name.c_str());
				ImGui::SameLine(150);
				ImGui::SetNextItemWidth(-1);
				if (ImGui::InputText("##string_input", buffer, sizeof(buffer))) {
					*vs = buffer;
					is_dirty = true;
				}
			}
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

			if (ImGui::SmallButton("Revert")) {
				// Revert this specific property override
				engineService.ExecuteOnEngineThread([this, field_name]() {
					auto world = Engine::GetWorld();
					for (auto& entity : world.get_all_entities()) {
						if (entity.get_uid() == m_SelectedEntityID && entity.all<PrefabComponent>()) {
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
			if (ImGui::IsItemHovered()) {
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
			engineService.add_component(engineService.m_cont->m_snapshot_entity_handle, type_id);
		}
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
		ImGui::SetTooltip("Multiplier for gravity (default: 1.00 m/s^2)");
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






