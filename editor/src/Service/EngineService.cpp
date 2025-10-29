#include "Service/EngineService.hpp"
#include "Editor.hpp"

void EngineContainerService::EngineContainer::engine_service() {
	//messagingSystem.Subscribe(MessageID::ENGINE_CORE_UPDATE_COMPLETE, nullptr, std::bind(&EngineContainer::engine_snapshot_callback,std::ref(*this)));
	Engine::InitInheritWindow("Default.yaml", Editor::GetInstance().GetWindowPtr());
	Engine::SetState(Engine::Info::State::Wait);
	while (!Engine::ShouldClose()) {
		while (!Engine::ShouldClose() && Engine::GetState() != Engine::Info::State::Wait) { //wait completely suspends the engine
			engine_snapshot_callback();
			engine_process_render_commands();
			Engine::BeginFrame();
			Engine::CoreUpdate();
			Engine::EndFrame();
			Engine::UpdateDebug();
			engine_process_picking();
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
	int i{};
	for (auto e : entities_rng) {
		m_entities_snapshot[i] = e.get_uuid();
		m_names_snapshot[i++] = e.name();
	}
	if (m_snapshot_entity_handle == ~0ull) {
		return;
	}
	ecs::entity inspected_entity{ m_snapshot_entity_handle };
	m_component_list_snapshot = inspected_entity.get_reflectible_components();

	// Snapshot frame data for editor viewport (thread-safe texture ID access)
	auto* sceneRenderer = Engine::GetRenderSystem().m_SceneRenderer.get();
	if (sceneRenderer) {
		auto& frameData = sceneRenderer->GetFrameData();
		if (frameData.editorResolvedBuffer && frameData.editorResolvedBuffer->GetFBOHandle() != 0) {
			m_frameDataSnapshot.editorTextureID = frameData.editorResolvedBuffer->GetColorAttachmentRendererID(0);
			m_frameDataSnapshot.isValid = true;
			m_frameDataSnapshot.deltaTime = frameData.deltaTime;
		} else {
			m_frameDataSnapshot.isValid = false;
		}
	} else {
		m_frameDataSnapshot.isValid = false;
	}
}

//can provide further abstractions in the future
void EngineContainerService::EngineContainer::engine_snapshot_writeback()
{
	std::lock_guard lg{ m_mtx };
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

void EngineContainerService::EngineContainer::request_picking(int screenX, int screenY, int viewportWidth, int viewportHeight) {
	std::lock_guard<std::mutex> lg{m_mtx};

	// Set up new request
	m_pickingRequest.screenX = screenX;
	m_pickingRequest.screenY = screenY;
	m_pickingRequest.viewportWidth = viewportWidth;
	m_pickingRequest.viewportHeight = viewportHeight;
	m_pickingRequest.isActive = true;
	m_pickingRequest.isComplete = false;

	// Clear previous results
	m_pickingRequest.hasHit = false;
	m_pickingRequest.objectID = 0;
}

bool EngineContainerService::EngineContainer::get_picking_result(PickingRequest& outResult) {
	std::lock_guard<std::mutex> lg{m_mtx};

	if (m_pickingRequest.isActive && m_pickingRequest.isComplete) {
		// Copy result
		outResult = m_pickingRequest;

		// Reset for next request
		m_pickingRequest.isActive = false;
		m_pickingRequest.isComplete = false;

		return true;
	}

	return false;
}

void EngineContainerService::EngineContainer::engine_process_picking() {
	std::lock_guard<std::mutex> lg{m_mtx};

	// Check if there's an active request to process
	if (!m_pickingRequest.isActive || m_pickingRequest.isComplete) {
		return;
	}

	// Perform picking on engine thread (correct OpenGL context)
	auto* sceneRenderer = Engine::GetRenderSystem().m_SceneRenderer.get();
	if (!sceneRenderer) {
		m_pickingRequest.isComplete = true;
		m_pickingRequest.hasHit = false;
		return;
	}

	// Enable picking mode
	sceneRenderer->EnablePicking(true);

	// Create query from request parameters
	PickingQuery query;
	query.screenX = m_pickingRequest.screenX;
	query.screenY = m_pickingRequest.screenY;
	query.viewportWidth = m_pickingRequest.viewportWidth;
	query.viewportHeight = m_pickingRequest.viewportHeight;

	// Execute picking
	PickingResult result = sceneRenderer->QueryObjectPicking(query);

	// Disable picking mode
	sceneRenderer->EnablePicking(false);

	// Store results in shared state
	m_pickingRequest.hasHit = result.hasHit;
	m_pickingRequest.objectID = result.objectID;
	m_pickingRequest.worldPosition = result.worldPosition;
	m_pickingRequest.depth = result.depth;
	m_pickingRequest.isComplete = true;
}

EngineContainerService::FrameDataSnapshot EngineContainerService::EngineContainer::get_frame_data_snapshot() {
	std::lock_guard<std::mutex> lg{m_mtx};
	return m_frameDataSnapshot;
}

void EngineContainerService::EngineContainer::submit_set_ambient_light(float r, float g, float b) {
	std::lock_guard<std::mutex> lg{m_mtx};
	RenderCommand cmd;
	cmd.type = RenderCommandType::SetAmbientLight;
	cmd.data.ambientLight.r = r;
	cmd.data.ambientLight.g = g;
	cmd.data.ambientLight.b = b;
	m_renderCommandQueue.push(cmd);
}

void EngineContainerService::EngineContainer::submit_set_debug_visualization(bool showAABBs) {
	std::lock_guard<std::mutex> lg{m_mtx};
	RenderCommand cmd;
	cmd.type = RenderCommandType::SetDebugVisualization;
	cmd.data.debugVis.showAABBs = showAABBs;
	m_renderCommandQueue.push(cmd);
}

void EngineContainerService::EngineContainer::engine_process_render_commands() {
	std::lock_guard<std::mutex> lg{m_mtx};

	while (!m_renderCommandQueue.empty()) {
		RenderCommand cmd = m_renderCommandQueue.front();
		m_renderCommandQueue.pop();

		auto* sceneRenderer = Engine::GetRenderSystem().m_SceneRenderer.get();
		if (!sceneRenderer) continue;

		switch (cmd.type) {
		case RenderCommandType::SetAmbientLight:
			sceneRenderer->SetAmbientLight(glm::vec3(
				cmd.data.ambientLight.r,
				cmd.data.ambientLight.g,
				cmd.data.ambientLight.b
			));
			break;

		case RenderCommandType::SetDebugVisualization:
			sceneRenderer->SetAABBVisualization(cmd.data.debugVis.showAABBs);
			break;
		}
	}
}