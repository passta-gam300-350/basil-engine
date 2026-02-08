/******************************************************************************/
/*!
\file   entity.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  ECS entity implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "ecs/internal/entity.h"
#include "ecs/internal/world.h"
#include "ecs/internal/reflection.h"
#include "Scene/Scene.hpp"

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
		(void)(reg.storage(1));
		return entity(get_world_handle(), static_cast<std::uint32_t>(new_entity));
	}

	std::vector<std::pair<std::uint32_t, std::unique_ptr<std::byte[]>>> entity::get_reflectible_components() const {
		entt::registry& reg{ world(get_world_handle()).impl.get_registry() };
		entt::entity base_entity = world::detail::entt_entity_cast(*this);
		std::vector<std::pair<std::uint32_t, std::unique_ptr<std::byte[]>>> reflectibles;
		//auto& internal_id_map{ ReflectionRegistry::InternalID() };
		auto& typeinfo_map{ ReflectionRegistry::types()};
		std::decay_t<decltype(typeinfo_map)>::iterator it2{};
		//std::decay_t<decltype(internal_id_map)>::iterator it{};
		for (auto&& [cmp_id, storage] : reg.storage()) {
			if (storage.contains(base_entity) && (it2 = typeinfo_map.find(cmp_id)) != typeinfo_map.end()/* && (it = internal_id_map.find(cmp_id)) != internal_id_map.end()*/) {
				//std::size_t sz{ typeinfo_map[it->second].size_of() };
				std::size_t sz{ typeinfo_map[cmp_id].size_of() };
				reflectibles.emplace_back(std::make_pair(cmp_id, std::make_unique<std::byte[]>(sz)));
				std::memcpy(reflectibles.back().second.get(), storage.value(base_entity), sz);
			}
		}
		return reflectibles;
	}

	void entity::destroy() {
		world(get_world_handle()).remove_entity(*this);
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

	rp::Guid entity::get_scene_handle() const {
		return get<SceneComponent>().m_scene_guid.m_guid;
	}

	//stable id in scene
	std::uint32_t entity::get_scene_uid() const {
		return get<SceneIDComponent>().m_scene_id;
	}

	void entity::enable() {
		add<active_t>();
	}
	void entity::disable() {
		remove<active_t>();
	}

	bool entity::is_active() const {
		return all<active_t>();
	}
}