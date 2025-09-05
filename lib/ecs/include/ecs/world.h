#ifndef LIB_ECS_WORLD_H
#define LIB_ECS_WORLD_H

#include <memory>
#include <entt/entt.hpp>
#include "ecs/fwd.h"

namespace ecs {

	namespace detail {
		using satisfactory_ecs_iterative_container_t = std::vector<int>;
	}

	struct world {
		using world_id_t = uint32_t;
		static constexpr world_id_t invalid_handle{ ~0x0UL };

		//Do not use. struct for implementation use only
		struct detail {
			entt::registry& get_registry();
			static entt::entity entt_entity_cast(ecs::entity);
			ecs::entity entity_cast(entt::entity);

			consteval detail() : handle{ world::invalid_handle } {}
			detail(world_id_t hdl) : handle{ hdl } {}
			detail(detail const& dt) : handle{ dt.handle } {}
			~detail() {}

			world_id_t handle;
		};

		static world new_world_instance();
		consteval world() = default;
		world(std::uint32_t hdl) : impl(hdl) {}
		world(world const& wrld) : impl(wrld.impl) {}

		entity add_entity();
		void remove_entity(entity);

		template <typename component_t, typename... c_args>
		component_t& add_component_to_entity(entity, c_args&&...);

		template <typename... component_ts>
		decltype(auto) get_component_from_entity(entity);

		template <typename... component_ts>
		void remove_component_from_entity(entity);

		template <typename... component_ts>
		bool has_any_components_in_entity(entity);

		template <typename... component_ts>
		bool has_all_components_in_entity(entity);

		template <typename... requires_t, typename... excludes_t>
		ecs_iterative_container decltype(auto) filter_entities(excludes_t...);

		world::detail impl;
	};

	void init_ecs();
	void shutdown_ecs();


	template<typename component_t, typename ...c_args>
	inline component_t& world::add_component_to_entity(entity enty, c_args&&... cargs)
	{
		return impl.get_registry().emplace<component_t>(detail::entt_entity_cast(enty), std::forward<c_args>(cargs)...);
	}

	template<typename... component_ts>
	inline decltype(auto) world::get_component_from_entity(entity enty)
	{
		return impl.get_registry().get<component_ts...>(detail::entt_entity_cast(enty));
	}

	template<typename ...component_ts>
	inline void world::remove_component_from_entity(entity enty)
	{
		impl.get_registry().remove(enty);
		impl.get_registry().view();
	}

	template<typename ...component_ts>
	inline bool world::has_any_components_in_entity(entity enty)
	{
		return impl.get_registry().any_of<component_ts...>(detail::entt_entity_cast(enty));
	}

	template<typename ...component_ts>
	inline bool world::has_all_components_in_entity(entity enty)
	{
		return impl.get_registry().all_of<component_ts...>(detail::entt_entity_cast(enty));
	}

	template <typename... requires_t, typename... excludes_t>
	inline ecs_iterative_container decltype(auto) world::filter_entities(excludes_t... ex)
	{
		return impl.handle, impl.get_registry().view<requires_t...>(ex...);
	}

}
#endif