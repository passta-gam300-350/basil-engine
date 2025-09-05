#include <vector>
#include "ecs/world.h"
#include "ecs/entity.h"
#include "ecs/utility.h"

namespace ecs {

	std::unique_ptr<std::vector<entt::registry>> worlds{};

	STRONG_INLINE std::uint64_t make_qword(std::uint32_t hi_dword, std::uint32_t low_dword) { return (static_cast<std::uint64_t>(hi_dword) << 32) | (low_dword); }

	entt::registry& get_world_registry(std::uint32_t wid)
	{
		return (*worlds)[wid];
	}

	entt::registry& get_world_registry(world wrld)
	{
		return (*worlds)[wrld.impl.handle];
	}

	entt::registry& world::detail::get_registry()
	{
		return get_world_registry(handle);
	}

	STRONG_INLINE entt::entity world::detail::entt_entity_cast(ecs::entity enty)
	{
		return static_cast<entt::entity>(enty.id);
	}

	ecs::entity world::detail::entity_cast(entt::entity entt_entity)
	{
		return ecs::entity{ handle, static_cast<std::uint32_t>(entt_entity) };
	}

	world world::new_world_instance()
	{
		std::uint32_t id{ static_cast<std::uint32_t>(worlds->size()) };
		worlds->emplace_back();
		return world(id);
	}

	entity world::add_entity()
	{
		entt::entity new_entity{ impl.get_registry().create() };
		return entity{ impl.handle, static_cast<std::uint32_t>(new_entity) };
	}

	void world::remove_entity(entity enty)
	{
		impl.get_registry().destroy(detail::entt_entity_cast(enty));
	}

	void init_ecs()
	{
		if (!worlds) {
			worlds.reset(new std::vector<entt::registry>{});
		}
	}

	void shutdown_ecs()
	{
		worlds.reset();
	}

}