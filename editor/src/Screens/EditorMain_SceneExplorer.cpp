#include "Screens/EditorMain.hpp"
#include "System/PrefabSystem.hpp"

#include "Render/Render.h"
#include "Component/MaterialOverridesComponent.hpp"
#include "Particles/ParticleComponent.h"
#include "Scene/Scene.hpp"


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

	auto RenderSceneGraphNode{ [this](const SceneGraphNode& node, rp::Guid scnguid) {
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

			int(*current_style)() { nullptr };

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

			// Add prefab prefix for visual distinction
			std::string displayName = node.m_entity_name;
			if (isPrefabInstance) {
				displayName = "[P] " + displayName;  // [P] indicates prefab instance
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
						if (!node.m_entity_handle) {
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
					else if (node.m_entity_handle && ImGui::MenuItem("Clear Parent")) {
						engineService.clear_parent_entity(node.m_entity_handle);
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Rename")) {}
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
			if (ImGui::MenuItem("Sphere")) {}
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
							}
							else {
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
	if ((ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) && ImGui::IsKeyPressed(ImGuiKey_G)) {
		engineService.create_parent_entity(m_EntitiesIDSelection);
	}
}
