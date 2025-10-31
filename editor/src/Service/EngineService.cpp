#include "Service/EngineService.hpp"
#include "Editor.hpp"
#include <Render/Render.h>  // For RenderSystem and FrameData
#include <algorithm>  // For std::find

void EngineContainerService::EngineContainer::engine_service() {
	//messagingSystem.Subscribe(MessageID::ENGINE_CORE_UPDATE_COMPLETE, nullptr, std::bind(&EngineContainer::engine_snapshot_callback,std::ref(*this)));
	Engine::InitInheritWindow("Default.yaml", Editor::GetInstance().GetWindowPtr());
	Engine::SetState(Engine::Info::State::Wait);
	while (!Engine::ShouldClose()) {
		while (!Engine::ShouldClose() && Engine::GetState() != Engine::Info::State::Wait) { //wait completely suspends the engine
			engine_snapshot_callback();
			Engine::BeginFrame();
			Engine::CoreUpdate();
			Engine::EndFrame();
			Engine::UpdateDebug();
			m_container_is_presentable.acquire();
			engine_snapshot_writeback();
		}
	}
	Engine::Exit();
	m_container_is_closed.release();
}

void EngineContainerService::EngineContainer::engine_snapshot_callback()
{
	std::lock_guard lg{ m_mtx };
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

		if (auto* storage = w.impl.get_registry().storage(ReflectionRegistry::types()[type_id].info().hash())) {
			auto it = std::find_if(m_component_list_snapshot.begin(), m_component_list_snapshot.end(), [type_id](std::pair<std::uint32_t, std::unique_ptr<std::byte[]>>& kvpair) { return kvpair.first == type_id; });
			if (storage->contains(ecs::world::detail::entt_entity_cast(inspected_entity)) && it != m_component_list_snapshot.end()) {
				std::memcpy(storage->value(ecs::world::detail::entt_entity_cast(inspected_entity)), it->second.get(), ReflectionRegistry::types()[type_id].size_of());
			}
		}
	}
	int create_ct = m_entity_create_count;
	m_entity_create_count = 0;
	while (create_ct-- > 0) {
		w.add_entity();
	}
	while (!m_entity_component_update_queue.empty()) {
		auto [ehdl, type_id, is_delete] = m_entity_component_update_queue.front();
		m_entity_component_update_queue.pop();
		ecs::entity entity{ ehdl };
		entt::entity enttntt{ ecs::world::detail::entt_entity_cast(entity) };
		if (auto* storage = w.impl.get_registry().storage(ReflectionRegistry::types()[type_id].info().index())) {
			if (!storage->contains(enttntt)) {
				continue;
			}
			if (is_delete) {
				storage->erase(enttntt);
			}
			else {
				storage->push(enttntt);
			}
		}
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
	m_cont->m_entity_component_update_queue.push(std::make_tuple(ehdl, component_type_id, false));
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

	std::lock_guard lg{ m_cont->m_mtx };
	return m_cont->m_entities_snapshot;
}

const std::vector<std::string>& EngineContainerService::GetEntityNamesSnapshot() const {
	static const std::vector<std::string> empty;
	if (!m_cont) return empty;

	std::lock_guard lg{ m_cont->m_mtx };
	return m_cont->m_names_snapshot;
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

		// Enable picking pass temporarily
		sceneRenderer->EnablePicking(true);

		// Create picking query with viewport-relative coordinates
		MousePickingQuery query;
		query.screenX = static_cast<int>(mouseX);
		query.screenY = static_cast<int>(mouseY);
		query.viewportWidth = static_cast<int>(viewportWidth);
		query.viewportHeight = static_cast<int>(viewportHeight);

		// Perform picking query
		PickingResult result = sceneRenderer->QueryObjectPicking(query);

		// Disable picking pass after use
		sceneRenderer->EnablePicking(false);

		// Return result via callback
		if (result.hasHit && result.objectID != 0) {
			spdlog::info("EngineService: Entity picked! Object ID: {}, World Position: ({:.2f}, {:.2f}, {:.2f})",
				result.objectID, result.worldPosition.x, result.worldPosition.y, result.worldPosition.z);
			if (resultCallback) resultCallback(true, result.objectID);
		}
		else {
			spdlog::info("EngineService: No entity picked");
			if (resultCallback) resultCallback(false, 0);
		}
	});
}

void EngineContainerService::EnableAABBVisualization(bool enable) {
	ExecuteOnEngineThread([enable]() {
		auto* sceneRenderer = Engine::GetRenderSystem().m_SceneRenderer.get();
		if (sceneRenderer) {
			sceneRenderer->EnableAABBVisualization(enable);
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
		ecs::world world = Engine::GetWorld();
		world.SaveYAML(path.c_str());
		spdlog::info("EngineService: Scene saved to {}", path);
	});
}

void EngineContainerService::LoadScene(const char* path) {
	if (!path) {
		spdlog::error("EngineService: Cannot load scene - path is null");
		return;
	}

	ExecuteOnEngineThread([path = std::string(path)]() {
		ecs::world world = Engine::GetWorld();
		world.UnloadAll();
		world.LoadYAML(path.c_str());
		spdlog::info("EngineService: Scene loaded from {}", path);
	});
}