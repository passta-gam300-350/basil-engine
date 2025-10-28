#include "Service/EngineService.hpp"
#include "Editor.hpp"
#include "Manager/MonoEntityManager.hpp"

#include <filesystem>
void EngineContainerService::EngineContainer::engine_service() {
	MonoEntityManager::GetInstance().initialize();

	std::string working_dir = Editor::GetInstance().GetConfig().project_workingDir;
	std::string asset_dir = working_dir + "assets";

	std::filesystem::path scripts_dir = std::filesystem::path{ working_dir } / "scripts";
	if (std::filesystem::exists(scripts_dir)) {
		MonoEntityManager::GetInstance().AddSearchDirectory(scripts_dir.string().c_str());
	}

	std::filesystem::path managed_dir = std::filesystem::path{ working_dir } / "managed";
	if (std::filesystem::exists(managed_dir)) {
		MonoEntityManager::GetInstance().SetOutputDirectory(managed_dir.string().c_str());
	}

	//Mono Configuration here
	


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