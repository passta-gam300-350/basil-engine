#include <vector>
#include "ecs/internal/world.h"
#include "ecs/internal/entity.h"
#include "ecs/utility/utility.h"
#include "ecs/system/scheduler.h"

namespace ecs {
	struct world_registry {
		std::unique_ptr<std::vector<entt::registry>> packed_worlds;
		std::vector<std::uint64_t> sparse_handles;
		std::uint64_t head;

		world new_world();
		void destroy_world(world);
	} world_reg;
}

namespace ecs {

	std::unique_ptr<std::vector<entt::registry>> worlds{};
	std::unique_ptr<std::vector<Scheduler>> world_systems{};

	STRONG_INLINE std::uint64_t make_qword(std::uint32_t hi_dword, std::uint32_t low_dword) { return (static_cast<std::uint64_t>(hi_dword) << 32) | (low_dword); }

	entt::registry& get_world_registry(std::uint32_t wid)
	{
		return (*worlds)[wid];
	}

	entt::registry& get_world_registry(world wrld)
	{
		return (*worlds)[wrld.impl.handle];
	}

	entt::registry& world::detail::get_registry() const
	{
		return get_world_registry(handle);
	}

	Scheduler& get_system_scheduler(world wrld)
	{
		return (*world_systems)[wrld.impl.handle];
	}

	Scheduler& world::detail::get_scheduler() const
	{
		return get_system_scheduler(handle);
	}

	STRONG_INLINE entt::entity world::detail::entt_entity_cast(ecs::entity enty)
	{
		return static_cast<entt::entity>(enty.get_uid());
	}

	STRONG_INLINE std::uint32_t world::detail::entity_id_cast(entt::entity entt_entity)
	{
		return static_cast<std::uint32_t>(entt_entity);
	}

	ecs::entity world::detail::entity_cast(entt::entity entt_entity) const
	{
		return ecs::entity{ handle, static_cast<std::uint32_t>(entt_entity) };
	}

	world world::new_world_instance()
	{
		std::uint32_t id{ static_cast<std::uint32_t>(worlds->size()) };
		worlds->emplace_back();
		world_systems->emplace_back();
		return world(id);
	}

	void world::destroy_world()
	{

	}

	entity world::add_entity()
	{
		entt::registry& reg{ impl.get_registry() };
		entt::entity new_entity{ reg.create() };
		reg.emplace<entity::entity_name_t>(new_entity, default_name);
		return entity{ impl.handle, static_cast<std::uint32_t>(new_entity) };
	}

	void world::remove_entity(entity enty)
	{
		impl.get_registry().destroy(detail::entt_entity_cast(enty));
	}

	void world::update(float dt)
	{
		impl.get_scheduler().run(*this);
	}

	void init_ecs()
	{
		if (!worlds) {
			worlds.reset(new std::vector<entt::registry>{});
		}
		if (!world_systems) {
			world_systems.reset(new std::vector<Scheduler>{});
		}
	}

	void shutdown_ecs()
	{
		worlds.reset();
		world_systems.reset();
	}

}