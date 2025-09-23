#include "ecs/internal/entity.h"
#include "ecs/internal/world.h"


namespace ecs {
	entity entity::duplicate() {
		entt::registry& reg{ world(get_world_handle()).impl.get_registry() };
		entt::entity base_entity = world::detail::entt_entity_cast(*this);
		entt::entity new_entity = reg.create();
		for (auto&& [cmp_id, storage] : reg.storage()) {
			if (storage.contains(base_entity)) {
				storage.push(new_entity, storage.value(base_entity));
			}
		}
		return entity(get_world_handle(), static_cast<std::uint32_t>(new_entity));
	}

	void entity::destroy() {
		world(get_world_handle()).impl.get_registry().destroy(world::detail::entt_entity_cast(*this));
		invalidate();
	}

	entity::operator bool() const {
		return WorldRegistry::Exists(world(get_world_handle())) && world(get_world_handle()).is_valid(*this);
	}

	std::string& entity::name() {
		return get<entity_name_t>().m_name;
	}
	std::string const& entity::name() const {
		return get<entity_name_t>().m_name;
	}
}