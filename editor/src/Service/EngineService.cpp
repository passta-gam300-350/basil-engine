#include "Service/EngineService.hpp"
#include "System/TransformSystem.hpp"
#include "Editor.hpp"
#include "Manager/MonoEntityManager.hpp"

#include <filesystem>

#include "Render/Render.h"
#include <Render/VideoComponent.hpp>
#include "System/Audio.hpp"
#include "Manager/ResourceSystem.hpp"
#include <Scene/Scene.hpp>
#include "Input/Button.h"
#include <Physics/Physics_Components.h>

#include "System/BehaviourSystem.hpp"
#include "Profiler/profiler.hpp"

void EngineContainerService::EngineContainer::engine_service() {
	MonoEntityManager::GetInstance().initialize();

	std::string working_dir = Editor::GetInstance().GetConfig().project_workingDir;
	std::string asset_dir = working_dir + "assets";

	std::filesystem::path scripts_dir = std::filesystem::path{ working_dir } / "assets"/ "scripts";
	if (std::filesystem::exists(scripts_dir)) {
		MonoEntityManager::GetInstance().AddSearchDirectory(scripts_dir.string().c_str());
		MonoEntityManager::GetInstance().SetProjectOutputDirectory(scripts_dir.string().c_str());
	}

	std::filesystem::path managed_dir = std::filesystem::path{ working_dir } / "managed";
	if (std::filesystem::exists(managed_dir)) {
		MonoEntityManager::GetInstance().SetOutputDirectory(managed_dir.string().c_str());
	}


	
	Engine::Instance().setWorkingDir(working_dir.c_str());
	



	//Mono Configuration here
	


	//messagingSystem.Subscribe(MessageID::ENGINE_CORE_UPDATE_COMPLETE, nullptr, std::bind(&EngineContainer::engine_snapshot_callback,std::ref(*this)));
	Engine::InitInheritWindow("Default.yaml", Editor::GetInstance().GetWindowPtr());
	Engine::Instance().GetSceneRegistry().SetSceneWorkingDir(asset_dir + "scenes/");
	std::string manifest_path = std::string{ Engine::getWorkingDir() } + "/scene_manifest.order";
	Engine::Instance().GetSceneRegistry().ReadManifest(manifest_path);

	// Enable HDR pipeline (disabled by default in SceneRenderer)
	Engine::GetRenderSystem().m_SceneRenderer->ToggleHDRPipeline(true);

	Engine::SetState(Engine::Info::State::Wait);
	while (!Engine::ShouldClose()) {
		while (!Engine::ShouldClose() && Engine::GetState() != Engine::Info::State::Wait && Engine::GetState() != Engine::Info::State::Pause && Engine::GetState() != Engine::Info::State::Init) { //wait completely suspends the engine
			// Begin profiling frame at the start of the entire iteration
			Engine::BeginFrame();

			{
				{
					PF_SCOPE("EngineWork");
					{
						PF_SYSTEM("Snapshot callback");
						engine_snapshot_callback();
					}
					Engine::CoreUpdate();
					Engine::UpdateDebug();
				}
			}

			// GPU synchronization: Ensure all rendering is complete before releasing semaphore
			// This prevents screen tearing when editor reads the framebuffer texture
			//glFinish();

			{
				PF_SYSTEM("EntityPicking");
				if (m_hasPickingQuery)
				{
					auto *sceneRenderer = Engine::GetRenderSystem().m_SceneRenderer.get();
					if (sceneRenderer)
					{
						// Create picking query
						MousePickingQuery query;
						query.screenX = static_cast<int>(m_pickingMouseX);
						query.screenY = static_cast<int>(m_pickingMouseY);
						query.viewportWidth = static_cast<int>(m_pickingViewportWidth);
						query.viewportHeight = static_cast<int>(m_pickingViewportHeight);

						// Query the picking buffer (picking pass already rendered)
						PickingResult result = sceneRenderer->QueryObjectPicking(query);

						// Disable picking pass for subsequent frames
						sceneRenderer->EnablePicking(false);

						// Handle result immediately (we're on engine thread)
						if (result.hasHit && result.objectID != 0)
						{
							spdlog::info("EngineService: Entity picked! Object ID: {}, World Position: ({:.2f}, {:.2f}, {:.2f})",
								result.objectID, result.worldPosition.x, result.worldPosition.y, result.worldPosition.z);

							// Set outline immediately
							sceneRenderer->ClearOutlinedObjects();
							sceneRenderer->AddOutlinedObject(result.objectID);

							// Notify editor via callback
							if (m_pickingCallback) m_pickingCallback(true, result.objectID);
						}
						else
						{
							spdlog::info("EngineService: No entity picked");
							sceneRenderer->ClearOutlinedObjects();
							if (m_pickingCallback) m_pickingCallback(false, 0);
						}
					}

					// Clear picking query flag
					m_hasPickingQuery = false;
					m_pickingCallback = nullptr;
				}
			}

			{
				PF_SYSTEM("WaitForEditor");
				m_container_is_presentable.acquire();
			}

			{
				PF_SYSTEM("Writeback");
				engine_snapshot_writeback();
			}

			// End profiling frame at the end of the entire iteration
			Engine::EndFrame();
		}
	}
	Engine::Exit();
	m_container_is_closed.release();
}

void EngineContainerService::EngineContainer::engine_snapshot_callback()
{
	std::lock_guard lg{ m_mtx };

	// Phase 2: Update editor camera snapshot for RenderSystem
	// Copy camera data from EditorMain (protected by m_mtx)
	RenderSystem::EditorCameraSnapshot renderSnapshot;
	renderSnapshot.position = m_editorCameraSnapshot.position;
	renderSnapshot.front = m_editorCameraSnapshot.front;
	renderSnapshot.up = m_editorCameraSnapshot.up;
	renderSnapshot.right = m_editorCameraSnapshot.right;
	renderSnapshot.fov = m_editorCameraSnapshot.fov;
	renderSnapshot.aspectRatio = m_editorCameraSnapshot.aspectRatio;
	renderSnapshot.nearPlane = m_editorCameraSnapshot.nearPlane;
	renderSnapshot.farPlane = m_editorCameraSnapshot.farPlane;
	renderSnapshot.isPerspective = m_editorCameraSnapshot.isPerspective;
	renderSnapshot.renderSceneViewport = m_editorCameraSnapshot.renderSceneViewport;
	renderSnapshot.renderGameViewport = m_editorCameraSnapshot.renderGameViewport;
	Engine::GetRenderSystem().SetEditorCameraSnapshot(renderSnapshot);

	ecs::world w{ Engine::GetWorld() };
	auto entities_rng{ w.get_all_entities() };
	m_entities_snapshot.resize(entities_rng.size_hint());
	m_names_snapshot.resize(entities_rng.size_hint());
	m_entity_components_snapshot.resize(entities_rng.size_hint());

	int i{};
	for (auto e : entities_rng) {
		m_entities_snapshot[i] = e.get_uuid();
		m_names_snapshot[i] = e.name();

		// Gather component type IDs for this entity
		auto components = e.get_reflectible_components();
		m_entity_components_snapshot[i].clear();
		for (auto& [typeID, data] : components) {
			m_entity_components_snapshot[i].push_back(typeID);
		}
		i++;
	}
	for (auto& [guid, scn] : Engine::GetSceneRegistry().GetAllScenes()) {
		m_loaded_scenes_scenegraph_snapshot[guid].second = true;
		if (!scn.IsStale()) {
			m_loaded_scenes_scenegraph_snapshot[guid].first = BuildSceneGraph(scn.GetSceneEntitites());
			m_loaded_scenes_scenegraph_snapshot[guid].first->m_entity_name = scn.SceneName();
		}
	}
	rp::Guid deletion_arr[16]{}; //no point create queue, scene deletions are rare
	i = 0;
	for (auto& [guid, kv] : m_loaded_scenes_scenegraph_snapshot) {
		if (i == 16) {
			break; //continue next loop
		}
		if (!kv.second) {
			deletion_arr[i++] = guid;
		}
	}

	while (i--) {
		m_loaded_scenes_scenegraph_snapshot.erase(deletion_arr[i]);
	}

	if (m_snapshot_entity_handle == ~0ull) {
		return;
	}

	ecs::entity inspected_entity{ m_snapshot_entity_handle };
	m_component_list_snapshot = inspected_entity.get_reflectible_components();
}

//can provide further abstractions in the future
void EngineContainerService::EngineContainer::engine_snapshot_writeback()
{
	std::lock_guard lg{ m_mtx };

	// Execute queued commands FIRST (EditorMain → Engine communication)
	while (!m_command_queue.empty()) {
		auto command = std::move(m_command_queue.front());
		m_command_queue.pop();

		try {
			command();  // Execute lambda on engine thread
		} catch (const std::exception& e) {
			spdlog::error("EngineService: Command execution failed: {}", e.what());
		}
	}

	// Existing writeback operations
	ecs::world w{ Engine::GetWorld() };
	ecs::entity inspected_entity{ m_snapshot_entity_handle };
	while (!m_write_back_queue.empty()) {
		ReflectionRegistry::TypeID type_id = m_write_back_queue.front();
		m_write_back_queue.pop();

		auto meta_type = ReflectionRegistry::types()[type_id];
		if (auto* storage = w.impl.get_registry().storage(meta_type.info().hash())) {
			auto it = std::find_if(m_component_list_snapshot.begin(), m_component_list_snapshot.end(), [type_id](std::pair<std::uint32_t, std::unique_ptr<std::byte[]>>& kvpair) { return kvpair.first == type_id; });
			if (storage->contains(ecs::world::detail::entt_entity_cast(inspected_entity)) && it != m_component_list_snapshot.end()) {
				void* dest = storage->value(ecs::world::detail::entt_entity_cast(inspected_entity));
				void* src = it->second.get();

				// Use entt meta assignment for proper copy (handles non-trivial types like unordered_map)
				entt::meta_any dest_any = meta_type.from_void(dest);
				entt::meta_any src_any = meta_type.from_void(src);

				if (meta_type.info().hash() == entt::type_hash<VideoComponent>::value()) {
					VideoComponent* dest_vid = static_cast<VideoComponent*>(dest);
					VideoComponent* src_vid = static_cast<VideoComponent*>(src);
					dest_vid->fullscreenMode = src_vid->fullscreenMode;
					dest_vid->backgroundColor = src_vid->backgroundColor;
				}

				if (meta_type.info().hash() == entt::type_hash<TransformComponent>::value()) {
					TransformComponent* dest_trans = static_cast<TransformComponent*>(dest);
					TransformComponent* src_trans = static_cast<TransformComponent*>(src);
					if (src_trans->m_Scale != dest_trans->m_Scale) {
						Engine::ResizeEntityPhysicsCollider(inspected_entity, src_trans->m_Scale, dest_trans->m_Scale);
					}
				}

				bool shouldFitCollider{};
				if (meta_type.info().hash() == entt::type_hash<MeshRendererComponent>::value()) {
					MeshRendererComponent* dest_mesh = static_cast<MeshRendererComponent*>(dest);
					MeshRendererComponent* src_mesh = static_cast<MeshRendererComponent*>(src);
					if (src_mesh->m_MeshGuid != dest_mesh->m_MeshGuid) {
						shouldFitCollider = true;
					}
				}

				// Assign using meta system (properly handles copy constructors)
				dest_any.assign(src_any);

				if (shouldFitCollider) {
					Engine::FitEntityColliderToMesh(inspected_entity);
				}

				if (meta_type.info().hash() == entt::type_hash<TransformComponent>::value()) {
					Engine::SyncEntityTransformToPhysics(inspected_entity);
				}

				// Special handling for AudioComponent: Load sound from GUID if needed
				if (meta_type.info().hash() == entt::type_hash<AudioComponent>::value()) {
					AudioComponent* audioComp = static_cast<AudioComponent*>(dest);
					//spdlog::info("AudioComponent writeback: soundHandle={}, GUID={}, isInitialized={}",
					//             audioComp->soundHandle, audioComp->audioAssetGuid.m_guid.to_hex(), audioComp->isInitialized);

					// Check if GUID is valid
					if (audioComp->audioAssetGuid.m_guid != rp::null_guid) {
						// Query ResourceRegistry to find what soundHandle SHOULD be loaded for this GUID
						ResourceRegistry::Entry* entry = ResourceRegistry::Instance().Pool(audioComp->audioAssetGuid);
						if (entry) {
							ResourceHandle handle = entry->m_Vt.m_Get(entry->m_Pool, audioComp->audioAssetGuid.m_guid);
							int* soundHandlePtr = static_cast<int*>(entry->m_Vt.m_Ptr(entry->m_Pool, handle));

							if (soundHandlePtr && *soundHandlePtr >= 0) {
								int expectedSoundHandle = *soundHandlePtr;

								// Detect GUID change: current soundHandle doesn't match what the GUID should load
								if (audioComp->soundHandle != expectedSoundHandle) {
									//spdlog::info("AudioComponent: GUID changed detected (current handle: {}, expected: {})",
									//             audioComp->soundHandle, expectedSoundHandle);

									// Unload old sound if one was loaded
									if (audioComp->soundHandle >= 0 && AudioSystem::GetInstance().IsInitialized()) {
										//spdlog::info("AudioComponent: Unloading old sound (handle: {})", audioComp->soundHandle);
										AudioSystem::GetInstance().UnloadSound(audioComp->soundHandle);
									}

									// Load new sound
									//spdlog::info("AudioComponent: Loading new sound from GUID (handle: {})", expectedSoundHandle);
									audioComp->soundHandle = expectedSoundHandle;
									audioComp->isInitialized = false;  // Reset initialization state
									audioComp->RefreshSoundInfo();
									//spdlog::info("AudioComponent: Loaded sound handle {} from GUID, isInitialized={}",
									//             audioComp->soundHandle, audioComp->isInitialized);
								} else {
									//spdlog::info("AudioComponent: Sound already correctly loaded (handle: {})", audioComp->soundHandle);
								}
							} else {
								//spdlog::warn("AudioComponent: Failed to load sound from GUID (invalid handle)");
							}
						} else {
							//spdlog::warn("AudioComponent: Failed to get resource pool for audio GUID");
						}
					} else {
						//spdlog::info("AudioComponent: Skipping load - GUID is null");

						// If GUID is null but we have a loaded sound, unload it
						if (audioComp->soundHandle >= 0 && AudioSystem::GetInstance().IsInitialized()) {
							//spdlog::info("AudioComponent: GUID is null, unloading current sound (handle: {})", audioComp->soundHandle);
							AudioSystem::GetInstance().UnloadSound(audioComp->soundHandle);
							audioComp->soundHandle = -1;
							audioComp->isInitialized = false;
						}
					}
				}

				// Trigger EnTT observers for components with update callbacks
				// Currently only MeshRendererComponent uses on_update observers
				if (meta_type.info().hash() == entt::type_hash<MeshRendererComponent>::value()) {
					// Trigger on_update<MeshRendererComponent> observer
					// This will call RenderSystem::OnMeshRendererUpdated()
					entt::entity enttntt = ecs::world::detail::entt_entity_cast(inspected_entity);
					MeshRendererComponent* dest_mesh = static_cast<MeshRendererComponent*>(dest);
					MeshRendererComponent* src_mesh = static_cast<MeshRendererComponent*>(src);
					dest_mesh->m_PrimitiveType = src_mesh->m_PrimitiveType;
					w.impl.get_registry().patch<MeshRendererComponent>(enttntt);
				}
			}
		}
	}
	int create_ct = m_entity_create_count;
	m_entity_create_count = 0;
	while (create_ct-- > 0) {
		w.add_entity();
	}
	// Process component addition/deletion queue
	//if (!m_entity_component_update_queue.empty()) {
	//	spdlog::info("EngineService: Processing component update queue, size={}",
	//	             m_entity_component_update_queue.size());
	//}

	while (!m_entity_component_update_queue.empty()) {
		auto [ehdl, type_id, is_delete] = m_entity_component_update_queue.front();
		m_entity_component_update_queue.pop();

		// Get component type name for logging
		//entt::meta_type meta = entt::resolve(static_cast<entt::id_type>(type_id));
		//std::string componentName = meta ? std::string(meta.info().name()) : "Unknown";

		//spdlog::info("EngineService: Processing component update - entity={}, type={}, type_id={}, is_delete={}",
		//             ehdl, componentName, type_id, is_delete);

		ecs::entity entity{ ehdl };
		entt::entity enttntt{ ecs::world::detail::entt_entity_cast(entity) };

		if (auto* storage = w.impl.get_registry().storage(type_id)) {
			//bool alreadyExists = storage->contains(enttntt);
			//spdlog::info("EngineService: Storage found for {}, component already exists={}",
			//             componentName, alreadyExists);

			if (!storage->contains(enttntt)) {
				if (type_id == entt::type_hash<TransformComponent>::value()) {
					inspected_entity.add<TransformComponent>(); //use specialization to handle transform mtx component
				}
				else {
					storage->push(enttntt);
					if (type_id == entt::type_hash<BoxCollider>::value() ||
						type_id == entt::type_hash<SphereCollider>::value() ||
						type_id == entt::type_hash<CapsuleCollider>::value() ||
						type_id == entt::type_hash<MeshCollider>::value())
						Engine::FitEntityColliderToMesh(entity);
				}
				//spdlog::info("EngineService: Added {} to entity {}", componentName, ehdl);
			}
			else {
				if (is_delete) {
					storage->erase(enttntt);
					//spdlog::info("EngineService: Deleted {} from entity {}", componentName, ehdl);
				}
				//else {
				//	spdlog::info("EngineService: Component {} already exists on entity {}, skipping add",
				//	             componentName, ehdl);
				//}
			}
		}
		else if (!is_delete) {
			auto metaIt = ReflectionRegistry::types().find(type_id);
			if (metaIt != ReflectionRegistry::types().end()) {
				auto emplaceFunc = metaIt->second.func("emplace"_tn);
				if (emplaceFunc) {
					emplaceFunc.invoke({}, entt::forward_as_meta(w.impl.get_registry()), enttntt);
				}
			}
			if (type_id == entt::type_hash<BoxCollider>::value() || 
				type_id == entt::type_hash<SphereCollider>::value() || 
				type_id == entt::type_hash<CapsuleCollider>::value() || 
				type_id == entt::type_hash<MeshCollider>::value())
				Engine::FitEntityColliderToMesh(entity);
		}
		//else {
		//	spdlog::warn("EngineService: No storage found for type {} (type_id={})",
		//	             componentName, type_id);
		//}
	}
	while (!m_entity_delete_queue.empty()) {
		auto ehdl = m_entity_delete_queue.front();
		m_entity_delete_queue.pop();
		ecs::entity entity{ ehdl };
		entity.destroy();
	}
	
}

void EngineContainerService::reset() {
	m_cont->m_container_is_presentable.release();
	Engine::SetState(Engine::Info::State::Exit);
	{
		std::lock_guard lg{ m_cont->m_mtx };
	}
	m_cont = std::move(std::make_unique<EngineContainer>());
}

void EngineContainerService::exit()
{
	if (!m_cont)
		return;
	m_cont->m_container_is_presentable.release();
	Engine::SetState(Engine::Info::State::Exit);
	{
		std::lock_guard lg{ m_cont->m_mtx };
	}
}

void EngineContainerService::release() {
	exit();
	m_cont.reset(nullptr);
}

void EngineContainerService::block() {
	if (!m_cont) {
		return;
	}
	Engine::SetState(Engine::Info::State::Wait);
	std::lock_guard lg{ m_cont->m_mtx };
}

void EngineContainerService::start() {
	Engine::SetState(Engine::Info::State::Running);
}

void EngineContainerService::run() {
	if (!m_cont) {
		m_cont = std::make_unique<EngineContainer>();
	}
	Engine::SetState(Engine::Info::State::Running);
}

void EngineContainerService::init() {
	if (!m_cont) {
		Engine::SetState(Engine::Info::State::Init);
		m_cont = std::make_unique<EngineContainer>();
	}
}

void EngineContainerService::pause() {
	Engine::SetState(Engine::Info::State::Pause);
}

//void EngineContainerService::create_cube() {
//}

void EngineContainerService::end() {
	release();
}

void EngineContainerService::create_entity() {
	if (!m_cont) {
		spdlog::warn("EngineService: Cannot create entity, engine container not initialized");
		return;
	}
	std::lock_guard lg{ m_cont->m_mtx };
	++m_cont->m_entity_create_count;
}

void EngineContainerService::create_child_entity(entity_handle parent)
{
	ecs::entity parentent = ecs::entity(parent);
	ExecuteOnEngineThread([parentent]() {
		ecs::entity child = ecs::world(parentent.get_world_handle()).add_entity();
		if (auto scene_res = Engine::GetSceneRegistry().GetScene(parentent.get_scene_handle()); scene_res) {
			scene_res.value().get().AddEntityToScene(child);
		}
		SceneGraph::AddChild(parentent, child);
		});
}

void EngineContainerService::orphan_children_entities(entity_handle parent)
{
	ecs::entity parentent = ecs::entity(parent);
	ExecuteOnEngineThread([parentent]() {
		auto children{ SceneGraph::GetChildren(parentent) };
		for (auto child : children) {
			SceneGraph::RemoveParent(child, true);
		}
		});
}

void EngineContainerService::create_parent_entity(entity_handle child)
{
	ecs::entity childent = ecs::entity(child);
	ExecuteOnEngineThread([childent]() {
		ecs::entity parent = ecs::world(childent.get_world_handle()).add_entity();
		SceneGraph::SetParent(childent, parent, true);
		parent.add<TransformComponent>();
		});
}

void EngineContainerService::create_parent_entity(std::unordered_set<std::uint32_t> const& children)
{
	ExecuteOnEngineThread([children]() {
		ecs::entity parent = ecs::world(Engine::GetWorld().impl.handle).add_entity();
		for (auto const& child : children) {
			ecs::entity childent = ecs::entity(parent.get_world_handle(), child);
			if (!childent)
				continue;
			SceneGraph::SetParent(childent, parent, true);
		}
		parent.add<TransformComponent>();
		});
}

void EngineContainerService::clear_parent_entity(entity_handle child)
{
	ecs::entity childent = ecs::entity(child);
	ExecuteOnEngineThread([childent]() {
		SceneGraph::RemoveParent(childent, true);
		});
}

void EngineContainerService::make_parent_entity(entity_handle parent, entity_handle child)
{
	ecs::entity parentent = ecs::entity(parent);
	ecs::entity childent = ecs::entity(child);
	ExecuteOnEngineThread([parentent, childent]() {
		SceneGraph::SetParent(childent, parentent, true);
		});
}

void EngineContainerService::delete_entity(entity_handle ehdl) {
	if (!m_cont) {
		spdlog::warn("EngineService: Cannot delete entity, engine container not initialized");
		return;
	}
	std::lock_guard lg{ m_cont->m_mtx };
	m_cont->m_entity_delete_queue.push(ehdl);
}

void EngineContainerService::add_component(entity_handle ehdl, std::uint32_t component_type_id) {
	if (!m_cont) {
		spdlog::warn("EngineService: Cannot add component, engine container not initialized");
		return;
	}
	std::lock_guard lg{ m_cont->m_mtx };

	// Get component type name for logging
	//entt::meta_type meta = entt::resolve(static_cast<entt::id_type>(component_type_id));
	//std::string componentName = meta ? std::string(meta.info().name()) : "Unknown";

	//spdlog::info("EngineService: Queueing component addition - entity={}, type={}, type_id={}",
	//             ehdl, componentName, component_type_id);

	m_cont->m_entity_component_update_queue.push(std::make_tuple(ehdl, component_type_id, false));
	//spdlog::info("EngineService: Component queued successfully, queue size={}",
	//             m_cont->m_entity_component_update_queue.size());
}

void EngineContainerService::delete_component(entity_handle ehdl, std::uint32_t component_type_id) {
	if (!m_cont) {
		spdlog::warn("EngineService: Cannot delete component, engine container not initialized");
		return;
	}
	std::lock_guard lg{ m_cont->m_mtx };
	m_cont->m_entity_component_update_queue.push(std::make_tuple(ehdl, component_type_id, true));
}

std::vector<std::pair<ReflectionRegistry::TypeID, std::string>>& EngineContainerService::get_reflectible_component_id_name_list() {
	static std::vector<std::pair<ReflectionRegistry::TypeID, std::string>> s_id_type_name_list{ [] () {
		std::vector<std::pair<ReflectionRegistry::TypeID, std::string>> temp{};
		auto& reg {Engine::GetWorld().impl.get_registry()};
		auto& type_reg{ ReflectionRegistry::types() };
		auto& type_name_reg{ ReflectionRegistry::TypeNames() };
		for (auto [id, storage] : reg.storage()) {
			if (auto it{ type_reg.find(storage.type().hash()) }; it != type_reg.end()) {
				temp.emplace_back(std::make_pair(it->first, type_name_reg[it->second.id()]));
			}
		}

		auto internalIt = ReflectionRegistry::InternalID().find(entt::type_index<Button>::value());
		if (internalIt != ReflectionRegistry::InternalID().end()) {
			const ReflectionRegistry::TypeID buttonTypeId = internalIt->second;
			const bool alreadyListed = std::find_if(temp.begin(), temp.end(),
				[buttonTypeId](const auto& entry) { return entry.first == buttonTypeId; }) != temp.end();

			if (!alreadyListed) {
				auto typeIt = type_reg.find(buttonTypeId);
				if (typeIt != type_reg.end()) {
					temp.emplace_back(buttonTypeId, type_name_reg[typeIt->second.id()]);
				}
			}
		}

		return temp;
	}()};
	return s_id_type_name_list;
}

// ============================================================================
// NEW IMPLEMENTATIONS: Command Queue & Query API
// ============================================================================

void EngineContainerService::ExecuteOnEngineThread(std::function<void()> command) {
	if (!m_cont) {
		spdlog::warn("EngineService: Cannot execute command, engine container not initialized");
		return;
	}

	std::lock_guard lg{ m_cont->m_mtx };
	m_cont->m_command_queue.push(std::move(command));
}

const std::vector<EngineContainerService::entity_handle>& EngineContainerService::GetEntitiesSnapshot() const {
	static const std::vector<entity_handle> empty;
	if (!m_cont) return empty;

	return m_cont->m_entities_snapshot;
}

const std::vector<std::string>& EngineContainerService::GetEntityNamesSnapshot() const {
	static const std::vector<std::string> empty;
	if (!m_cont) return empty;

	return m_cont->m_names_snapshot;
}

const std::unordered_map<rp::Guid, std::pair<std::shared_ptr<SceneGraphNode>, bool>>& EngineContainerService::GetSceneGraphSnapshot() const
{
	static const std::unordered_map<rp::Guid, std::pair<std::shared_ptr<SceneGraphNode>, bool>> empty{};
	return m_cont ? m_cont->m_loaded_scenes_scenegraph_snapshot : empty;
}

bool EngineContainerService::EntityHasComponent(entity_handle entityHandle, std::uint32_t componentTypeID) const {
	if (!m_cont) return false;

	std::lock_guard lg{ m_cont->m_mtx };

	// Find the entity in the snapshot
	for (size_t i = 0; i < m_cont->m_entities_snapshot.size(); ++i) {
		if (m_cont->m_entities_snapshot[i] == entityHandle) {
			// Check if this component type ID is in the entity's component list
			const auto& components = m_cont->m_entity_components_snapshot[i];
			return std::find(components.begin(), components.end(), componentTypeID) != components.end();
		}
	}

	return false; // Entity not found in snapshot
}

FrameData& EngineContainerService::GetFrameData() {
	// Safe: FrameData access is synchronized by m_container_is_presentable semaphore
	// Editor reads after engine releases semaphore
	return Engine::GetRenderSystem().m_SceneRenderer->GetFrameData();
}

Engine::Info EngineContainerService::GetEngineInfo() {
	return Engine::Instance().GetInfo();  // Returns copy, thread-safe
}

double EngineContainerService::GetDeltaTime() {
	return Engine::GetDeltaTime();
}

// ============================================================================
// HIGH-LEVEL API IMPLEMENTATIONS: Complex Operations
// ============================================================================

void EngineContainerService::SelectEntityByObjectID(uint32_t objectID) {
	ExecuteOnEngineThread([objectID, this]() {
		ecs::entity entity(uint32_t(Engine::GetWorld()), objectID);
		inspect_entity(entity.get_uuid());
	});
}

void EngineContainerService::PerformEntityPicking(float mouseX, float mouseY, float viewportWidth, float viewportHeight,
                                                    std::function<void(bool hasHit, uint32_t objectID)> resultCallback) {
	ExecuteOnEngineThread([mouseX, mouseY, viewportWidth, viewportHeight, resultCallback, this]() {
		auto* sceneRenderer = Engine::GetRenderSystem().m_SceneRenderer.get();
		if (!sceneRenderer) {
			spdlog::error("EngineService: SceneRenderer not available for entity picking");
			if (resultCallback) resultCallback(false, 0);
			return;
		}

		spdlog::info("EngineService: Performing entity picking at viewport position ({:.1f}, {:.1f}) in viewport ({:.1f}x{:.1f})",
			mouseX, mouseY, viewportWidth, viewportHeight);

		// Debug: Check how many entities exist and have renderable components
		ecs::world world = Engine::GetWorld();
		auto renderableCount = world.filter_entities<MeshRendererComponent, TransformComponent, VisibilityComponent>().size();
		auto totalEntities = m_cont->m_entities_snapshot.size();

		spdlog::info("EngineService: World has {} total entities, {} with renderable components", totalEntities, renderableCount);

		if (renderableCount == 0) {
			spdlog::warn("EngineService: No renderable entities found - nothing to pick");
			if (resultCallback) resultCallback(false, 0);
			return;
		}

		// The main engine loop will enable picking, render normally, then query after render
		m_cont->m_hasPickingQuery = true;
		m_cont->m_pickingMouseX = mouseX;
		m_cont->m_pickingMouseY = mouseY;
		m_cont->m_pickingViewportWidth = viewportWidth;
		m_cont->m_pickingViewportHeight = viewportHeight;
		m_cont->m_pickingCallback = resultCallback;

		// Enable picking pass for next frame
		sceneRenderer->EnablePicking(true);

	});
}

void EngineContainerService::AddOutlinedObject(uint32_t objectID) {
	ExecuteOnEngineThread([objectID]() {
		auto* sceneRenderer = Engine::GetRenderSystem().m_SceneRenderer.get();
		if (sceneRenderer) {
			sceneRenderer->AddOutlinedObject(objectID);
		}
	});
}

void EngineContainerService::RemoveOutlinedObject(uint32_t objectID) {
	ExecuteOnEngineThread([objectID]() {
		auto* sceneRenderer = Engine::GetRenderSystem().m_SceneRenderer.get();
		if (sceneRenderer) {
			sceneRenderer->RemoveOutlinedObject(objectID);
		}
	});
}

void EngineContainerService::ClearOutlinedObjects() {
	ExecuteOnEngineThread([]() {
		auto* sceneRenderer = Engine::GetRenderSystem().m_SceneRenderer.get();
		if (sceneRenderer) {
			sceneRenderer->ClearOutlinedObjects();
		}
	});
}

void EngineContainerService::SetAmbientLight(const glm::vec3& color) {
	ExecuteOnEngineThread([color]() {
		auto* sceneRenderer = Engine::GetRenderSystem().m_SceneRenderer.get();
		if (sceneRenderer) {
			sceneRenderer->SetAmbientLight(color);
			spdlog::info("EngineService: Ambient light set to ({:.2f}, {:.2f}, {:.2f})", color.x, color.y, color.z);
		}
	});
}

void EngineContainerService::SaveScene(const char* path) {
	if (!path) {
		spdlog::error("EngineService: Cannot save scene - path is null");
		return;
	}

	ExecuteOnEngineThread([path = std::string(path)]() {
		auto activeSceneOpt = Engine::GetSceneRegistry().GetActiveScene();
		if (activeSceneOpt.has_value()) {
			activeSceneOpt.value().get().SerializeYaml(path);
			spdlog::info("EngineService: Scene saved to {}", path);
		} else {
			spdlog::error("EngineService: No active scene to save!");
		}
	});
}

void EngineContainerService::LoadScene(const char* path) {
	if (!path) {
		spdlog::error("EngineService: Cannot load scene - path is null");
		return;
	}

	ExecuteOnEngineThread([path = std::string(path)]() {
		// Clear existing scenes
		ecs::world world = Engine::GetWorld();
		world.UnloadAll();

		// Load scene using SceneRegistry::LoadSceneFromPath which includes render settings (skybox, etc.)
		auto sceneRefOpt = Engine::GetSceneRegistry().LoadSceneFromPath(path);
		if (sceneRefOpt.has_value()) {
			spdlog::info("EngineService: Scene loaded from {}", path);
		} else {
			spdlog::error("EngineService: Failed to load scene from {}", path);
		}
		TransformSystem().FixedUpdate(world);
		Engine::GetRenderSystem().BuildBVH(world);
	});
}

void EngineContainerService::NewScene() {
	ExecuteOnEngineThread([this]() {
		ecs::world world = Engine::GetWorld();
		world.UnloadAll();
		m_cont->m_loaded_scenes_scenegraph_snapshot.clear();
		SceneGraphNode root{};
		root.m_entity_name = "Scene";
		m_cont->m_loaded_scenes_scenegraph_snapshot[rp::null_guid].first = std::make_shared<SceneGraphNode>(root);
		m_cont->m_loaded_scenes_scenegraph_snapshot[rp::null_guid].second = true;
		spdlog::info("EngineService: New Scene created");
		TransformSystem().FixedUpdate(world);
		Engine::GetRenderSystem().BuildBVH(world);
		Engine::GetRenderSystem().BuildInteractableBVH(world);
		});
}

void EngineContainerService::set_on_load()
{
	ExecuteOnEngineThread([]
	{
		Engine::SetOnLoadCallBack([]([[maybe_unused]] ecs::world& w)
		{
			BehaviourSystem::Instance().Reload();
			
		});
	});
}

void EngineContainerService::set_on_unload()
{
	ExecuteOnEngineThread([]
	{
		Engine::SetOnUnloadCallBack([]([[maybe_unused]] ecs::world& w)
		{
			
			BehaviourSystem::Instance().firstRun = true;
			BehaviourSystem::Instance().unloaded = true;
		});
	});
}
