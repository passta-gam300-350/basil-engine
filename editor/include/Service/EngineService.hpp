#ifndef EngineService_HPP
#define EngineService_HPP

#include <Engine.hpp>
#include <thread>
#include <semaphore>
#include <spdlog/spdlog.h>
#include "Service.hpp"

struct EngineContainerService : public Service
{
public:

	using entity_handle = std::uint64_t;

	struct EngineContainer {
		std::mutex m_mtx;
		std::jthread m_thread;
		std::vector<entity_handle> m_entities_snapshot;
		std::vector<std::string> m_names_snapshot;
		std::queue<std::uint32_t> m_write_back_queue;
		std::vector<std::pair<std::uint32_t, std::unique_ptr<std::byte[]>>> m_component_list_snapshot;
		entity_handle m_snapshot_entity_handle{ ~0ull };
		std::binary_semaphore m_container_is_closed{ 0 };
		std::binary_semaphore m_container_is_presentable{ 0 };
		std::queue<std::function<void()>> m_main_thread_tasks;

	private:
		void engine_service();
		void engine_snapshot_callback();
		void engine_snapshot_writeback();

	public:
		EngineContainer() : m_mtx{}, m_thread{ &EngineContainer::engine_service, std::ref(*this) } {}
		~EngineContainer() {
			Engine::SetState(Engine::Info::State::Exit);
			m_container_is_closed.acquire();
		}
	};
	std::unique_ptr<EngineContainer> m_cont{};
	void reset();
	void exit();
	void release();
	void block();
	void start();
	void run();
	void init();
	void pause();
	void create_cube();

	void end();
	void inspect_entity(entity_handle ehdl) { if (m_cont) m_cont->m_snapshot_entity_handle = ehdl; else spdlog::info("engine container is empty."); }
	void create_entity();
	void delete_entity(entity_handle);
	void add_component(entity_handle, std::uint32_t);
	void delete_component(entity_handle, std::uint32_t);
	

	//safe to return reference, registration is done during startup with its data determined during compile time and reflection registry is not expected to change, unless reset is called. 
	std::vector<std::pair<ReflectionRegistry::TypeID, std::string>>& get_reflectible_component_id_name_list();

};
#endif // FileService_HPP