#ifndef EngineService_HPP
#define EngineService_HPP

#include <Engine.hpp>
#include <thread>
#include <semaphore>
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include "Service.hpp"

struct EngineContainerService : public Service
{
public:

	using entity_handle = std::uint64_t;

	struct PickingRequest {
		// Input parameters
		int screenX{0};
		int screenY{0};
		int viewportWidth{0};
		int viewportHeight{0};

		// State flags
		bool isActive{false};    // Request is pending
		bool isComplete{false};  // Result is ready

		// Output results (uses Engine's PickingResultData internally)
		bool hasHit{false};
		uint32_t objectID{0};
		glm::vec3 worldPosition{0.0f};
		float depth{0.0f};
	};

	enum class RenderCommandType {
		SetAmbientLight,
		SetDebugVisualization,
		SetSelectedEntity,
		ClearSelectedEntity,
		SetEditorViewportSize
	};

	struct RenderCommand {
		RenderCommandType type;
		union {
			struct {
				float r, g, b;
			} ambientLight;
			struct {
				bool showAABBs;
			} debugVis;
			struct {
				uint32_t entityID;
			} selectedEntity;
			struct {
				int width, height;
			} viewportSize;
		} data;
	};

	struct EngineContainer {
		std::mutex m_mtx;
		std::jthread m_thread;
		std::vector<entity_handle> m_entities_snapshot;
		std::vector<std::string> m_names_snapshot;
		std::queue<std::uint32_t> m_write_back_queue;
		std::queue<entity_handle> m_entity_delete_queue;
		int m_entity_create_count;
		std::queue<std::tuple<entity_handle, std::uint32_t, bool>> m_entity_component_update_queue;
		std::vector<std::pair<std::uint32_t, std::unique_ptr<std::byte[]>>> m_component_list_snapshot;
		entity_handle m_snapshot_entity_handle{ ~0ull };
		std::binary_semaphore m_container_is_closed{ 0 };
		std::binary_semaphore m_container_is_presentable{ 0 };
		PickingRequest m_pickingRequest;
		FrameTextureData m_frameDataSnapshot;  // Uses Engine's FrameTextureData
		std::queue<RenderCommand> m_renderCommandQueue;

	private:
		void engine_service();
		void engine_snapshot_callback();
		void engine_snapshot_writeback();
		void engine_process_picking();
		void engine_process_render_commands();

	public:
		EngineContainer() : m_mtx{}, m_thread{ &EngineContainer::engine_service, std::ref(*this) } {}
		~EngineContainer() {
			Engine::SetState(Engine::Info::State::Exit);
			m_container_is_closed.acquire();
		}

		// Request object picking (called from editor thread)
		void request_picking(int screenX, int screenY, int viewportWidth, int viewportHeight);

		// Get picking result if ready (called from editor thread)
		// Returns true if result is available
		bool get_picking_result(PickingRequest& outResult);

		// Get frame data snapshot (called from editor thread)
		FrameTextureData get_frame_data_snapshot();

		// Submit render commands (called from editor thread)
		void submit_set_ambient_light(float r, float g, float b);
		void submit_set_debug_visualization(bool showAABBs);
		void submit_set_selected_entity(uint32_t entityID);
		void submit_clear_selected_entity();
		void submit_set_editor_viewport_size(int width, int height);
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