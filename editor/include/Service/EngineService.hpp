#ifndef EngineService_HPP
#define EngineService_HPP

#include <Engine.hpp>
#include <thread>
#include <semaphore>
#include <functional>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include "Service.hpp"

#include <cont/queue.hpp>
#include <Scene/SceneGraph.hpp>

// Forward declarations to reduce coupling
struct FrameData;

struct SceneGraphNode{
	SceneGraphNode* m_parent;
	std::vector<SceneGraphNode> m_children;

	std::uint64_t m_entity_handle;
	std::uint32_t m_entity_sid;
	std::string m_entity_name;
	bool is_active;
};

inline SceneGraphNode BuildSceneGraph(std::vector<ecs::entity> const& vec_e) {
	std::unordered_map<ecs::entity, bool> visited{};
	std::unordered_map<ecs::entity, SceneGraphNode> orphan{}; //waiting to get picked up by parents
	SceneGraphNode root{};

	auto buildscenegraphnode{ [&visited, &orphan](ecs::entity sentity) {
		auto visit_entity{ [&visited, &orphan](ecs::entity se, auto&& fn) -> SceneGraphNode {
			visited[se] = true;
			if (orphan.find(se) != orphan.end()) {
				return orphan[se];
			}
			SceneGraphNode& sgph{ orphan.emplace(se, SceneGraphNode{}).first->second }; //make sure that scene does not contain circular dependencies
			auto children = SceneGraph::GetChildren(se);
			for (auto child : children) {
				sgph.m_children.emplace_back(fn(child, fn));
			}
			sgph.m_entity_name = se.name();
			sgph.m_entity_handle = se.get_uuid();
			sgph.m_entity_sid = se.get_scene_uid();
			sgph.is_active = se.is_active();
			return sgph;
			}};
		return visit_entity(sentity, visit_entity);
		}};

	for (auto const& e : vec_e) {
		if (visited[e])
			continue;
		auto sgraph = buildscenegraphnode(e);
		if (!SceneGraph::HasParent(e)) {
			root.m_children.emplace_back(sgraph);
		}
	}

	return root;
}

inline std::shared_ptr<SceneGraphNode> BuildSceneGraph(std::unordered_map<std::uint32_t, ecs::entity> const& map_guid_e) {
	std::unordered_map<ecs::entity, bool> visited{};
	std::unordered_map<ecs::entity, SceneGraphNode> orphan{}; //waiting to get picked up by parents
	std::shared_ptr<SceneGraphNode> root{std::make_shared<SceneGraphNode>()};
	bool is_scene_active{};

	auto linkscenegraphnode{ [](SceneGraphNode& rootnd) {
		auto visit_node{ [](SceneGraphNode& parent, auto&& fn) -> void {
		for (auto& child : parent.m_children) {
			child.m_parent = &parent;
			fn(child, fn);
		}
		} }; 
		visit_node(rootnd, visit_node);
		}
	};
	auto buildscenegraphnode{ [&visited, &orphan, &is_scene_active](ecs::entity sentity) {
		auto visit_entity{ [&visited, &orphan, &is_scene_active](ecs::entity se, auto&& fn) -> SceneGraphNode {
			visited[se] = true;
			if (orphan.find(se) != orphan.end()) {
				return orphan[se];
			}
			SceneGraphNode& sgph{ orphan.emplace(se, SceneGraphNode{}).first->second }; //make sure that scene does not contain circular dependencies
			auto children = SceneGraph::GetChildren(se);
			for (auto child : children) {
				sgph.m_children.emplace_back(fn(child, fn));
			}
			sgph.m_entity_name = se.name();
			sgph.m_entity_handle = se.get_uuid();
			sgph.m_entity_sid = se.get_scene_uid();
			sgph.is_active = se.is_active();
			is_scene_active |= sgph.is_active;
			return sgph;
			}};
		return visit_entity(sentity, visit_entity);
		} };

	for (auto const& [sid, e] : map_guid_e) {
		if (visited[e])
			continue;
		auto sgraph = buildscenegraphnode(e);
		if (!SceneGraph::HasParent(e)) {
			root->m_children.emplace_back(sgraph);
		}
	}
	linkscenegraphnode(*root);

	root->is_active = is_scene_active;

	return root;
}

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
		std::queue<entity_handle> m_entity_delete_queue;
		int m_entity_create_count;
		std::queue<std::tuple<entity_handle, std::uint32_t, bool>> m_entity_component_update_queue;
		std::vector<std::pair<std::uint32_t, std::unique_ptr<std::byte[]>>> m_component_list_snapshot;
		entity_handle m_snapshot_entity_handle{ ~0ull };
		std::binary_semaphore m_container_is_closed{ 0 };
		std::binary_semaphore m_container_is_presentable{ 0 };

		// Command queue for EditorMain → Engine communication
		std::queue<std::function<void()>> m_command_queue;

		// Component type IDs per entity (parallel array to m_entities_snapshot)
		// Each vector contains the component type IDs that entity has
		std::vector<std::vector<std::uint32_t>> m_entity_components_snapshot;


		// Pending picking query data (processed after next frame render)
		bool m_hasPickingQuery{ false };
		float m_pickingMouseX{ 0.0f };
		float m_pickingMouseY{ 0.0f };
		float m_pickingViewportWidth{ 0.0f };
		float m_pickingViewportHeight{ 0.0f };
		std::function<void(bool, uint32_t)> m_pickingCallback;

		//make this shared ptr and called it a day. dont waste time on this
		std::unordered_map<rp::Guid, std::pair<std::shared_ptr<SceneGraphNode>, bool>> m_loaded_scenes_scenegraph_snapshot;

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
	void create_child_entity(entity_handle parent);
	void orphan_children_entities(entity_handle parent);
	void create_parent_entity(entity_handle child);
	void create_parent_entity(std::unordered_set<std::uint32_t> const& children);
	void clear_parent_entity(entity_handle child);
	void make_parent_entity(entity_handle parent, entity_handle child);
	void delete_entity(entity_handle);
	void add_component(entity_handle, std::uint32_t);
	void delete_component(entity_handle, std::uint32_t);


	void set_on_load();
	void set_on_unload();
	
	//safe to return reference, registration is done during startup with its data determined during compile time and reflection registry is not expected to change, unless reset is called.
	std::vector<std::pair<ReflectionRegistry::TypeID, std::string>>& get_reflectible_component_id_name_list();

	// ============================================================================
	// NEW API: Command Queue & Thread-Safe Queries
	// ============================================================================

	/**
	 * @brief Execute a command on the engine thread during the next writeback cycle
	 *
	 * This is the primary decoupling mechanism. EditorMain queues lambdas that will
	 * execute on the engine thread, allowing safe access to Engine APIs.
	 *
	 * @param command Lambda or function to execute (captured by value for thread safety)
	 *
	 * @example
	 * // Set ambient light
	 * engineService.ExecuteOnEngineThread([]() {
	 *     Engine::GetRenderSystem().m_SceneRenderer->SetAmbientLight(glm::vec3(0.7f));
	 * });
	 *
	 * // Create entity with components
	 * engineService.ExecuteOnEngineThread([position, scale]() {
	 *     ecs::world world = Engine::GetWorld();
	 *     auto entity = world.add_entity();
	 *     entity.add_component<TransformComponent>(position, glm::quat(), scale);
	 * });
	 */
	void ExecuteOnEngineThread(std::function<void()> command);

	/**
	 * @brief Get snapshot of all entity handles (updated each frame)
	 * Thread-safe: Returns const reference to main thread snapshot
	 */
	const std::vector<entity_handle>& GetEntitiesSnapshot() const;

	/**
	 * @brief Get snapshot of all entity names (updated each frame)
	 * Thread-safe: Returns const reference to main thread snapshot
	 */
	const std::vector<std::string>& GetEntityNamesSnapshot() const;

	/**
	 * @brief Get snapshot of all entity names (updated each frame)
	 * Thread-safe: Returns const reference to main thread snapshot
	 */
	const std::unordered_map<rp::Guid, std::pair<std::shared_ptr<SceneGraphNode>, bool>>& GetSceneGraphSnapshot() const;

	/**
	 * @brief Check if an entity has a specific component (from snapshot)
	 * Thread-safe: Queries snapshot data
	 * @param entityHandle Entity to check
	 * @param componentTypeID Component type ID from ReflectionRegistry
	 * @return True if entity has the component, false otherwise
	 */
	bool EntityHasComponent(entity_handle entityHandle, std::uint32_t componentTypeID) const;

	/**
	 * @brief Get frame data containing render targets and textures
	 * Thread-safe: Synchronized by semaphore (editor reads, engine writes)
	 * @return FrameData& containing editorResolvedBuffer for viewport rendering
	 */
	FrameData& GetFrameData();

	/**
	 * @brief Get current engine info (FPS, delta time, frame count)
	 * Thread-safe: Returns copy of info struct
	 */
	Engine::Info GetEngineInfo();

	/**
	 * @brief Get delta time for frame-rate independent operations
	 * Thread-safe: Read-only access to atomic/synchronized value
	 */
	double GetDeltaTime();

	// ============================================================================
	// HIGH-LEVEL API: Complex Operations (Lambdas in EngineService.cpp)
	// ============================================================================

	/**
	 * @brief Select an entity by its object ID (executes on engine thread)
	 *
	 * @param objectID The object ID to select
	 */
	void SelectEntityByObjectID(uint32_t objectID);

	/**
	 * @brief Perform entity picking at mouse position (executes on engine thread)
	 *
	 * @param mouseX Mouse X coordinate in viewport space
	 * @param mouseY Mouse Y coordinate in viewport space
	 * @param viewportWidth Viewport width
	 * @param viewportHeight Viewport height
	 * @param resultCallback Callback with picking result (hasHit, objectID)
	 */
	void PerformEntityPicking(float mouseX, float mouseY, float viewportWidth, float viewportHeight,
	                          std::function<void(bool hasHit, uint32_t objectID)> resultCallback);

	/**
	 * @brief Add object outline for visual selection feedback
	 * @param objectID Object ID to outline
	 */
	void AddOutlinedObject(uint32_t objectID);

	/**
	 * @brief Remove object outline
	 * @param objectID Object ID to remove outline from
	 */
	void RemoveOutlinedObject(uint32_t objectID);

	/**
	 * @brief Clear all outlined objects
	 */
	void ClearOutlinedObjects();

	/**
	 * @brief Set ambient light color for the scene
	 * @param color RGB color vector (0.0-1.0 range)
	 */
	void SetAmbientLight(const glm::vec3& color);

	/**
	 * @brief Save current scene to file (executes on engine thread)
	 * @param path File path to save to
	 */
	void SaveScene(const char* path);

	/**
	 * @brief Load scene from file (executes on engine thread)
	 * @param path File path to load from
	 */
	void LoadScene(const char* path);

	/**
	 * @brief creates a new scene (executes on engine thread)
	 * @param path File path to load from
	 */
	void NewScene();
};
#endif // FileService_HPP