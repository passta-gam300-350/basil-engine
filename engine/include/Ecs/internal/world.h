/******************************************************************************/
/*!
\file   world.h
\author Team PASSTA
		Chew Bangxin Steven (banxginsteven.chew@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/09
\brief    Declares the world class

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef LIB_ECS_WORLD_H
#define LIB_ECS_WORLD_H

#include <memory>
#include <entt/entt.hpp>
#include "ecs/fwd.h"
#include "ecs/internal/entity.h"
#include "Component/Transform.hpp"
#include <jobsystem.hpp>

namespace ecs {

	namespace detail {
		using satisfactory_ecs_iterative_container_t = std::vector<int>;
	}

	struct world {
		using world_id_t = uint32_t;
		static constexpr world_id_t invalid_handle{ null_handle31 };

		//Do not use. struct for implementation use only
		struct detail {
			entt::registry& get_registry() const;
			Scheduler& get_scheduler() const;
			static entt::entity entt_entity_cast(ecs::entity);
			static std::uint32_t entity_id_cast(entt::entity);
			ecs::entity entity_cast(entt::entity) const;

			detail() : handle{ world::invalid_handle } {}
			detail(world_id_t hdl) : handle{ hdl } {}
			detail(detail const& dt) : handle{ dt.handle } {}
			~detail() {}

			bool operator!=(detail other) const {
				return handle != other.handle;
			}
			bool operator==(detail other) const {
				return handle == other.handle;
			}

			world_id_t handle;
		};

		struct Scene {

		};

		//this for world dump. binary format use scene serialiser for friendly format
		void serialise_world_bin(std::string const& outputFilename);
		void deserialise_world_bin(std::string const& inputFilename);

		void LoadYAML(std::string const&);
		void SaveYAML(std::string const&);
		void UnloadNonGlobals();
		void UnloadAll();

		static world new_world_instance();
		world() = default;
		world(std::uint32_t hdl) : impl(hdl) {}
		world(world const& wrld) : impl(wrld.impl) {}

		operator bool() const;
		operator std::uint32_t() const {
			return impl.handle;
		}

		bool operator==(world other) {
			return other.impl == impl;
		}
		bool operator!=(world other) {
			return other.impl != impl;
		}

		//depreciated
		world copy(world);
		world inplace_union_world(world);
		world inplace_intersect_world(world);

		void destroy_world();
		void update(float);
		void update();
		void pre_update();
		JobID update_async();

		entity migrate_entity(entity);
		entity add_entity();
		void remove_entity(entity);

		template <typename component_t, typename... c_args>
		component_t& add_component_to_entity(entity, c_args&&...);

		template <typename... component_ts>
		decltype(auto) get_component_from_entity(entity);

		template <typename... component_ts>
		void remove_component_from_entity(entity);

		template <typename... component_ts>
		bool has_any_components_in_entity(entity) const;

		template <typename... component_ts>
		bool has_all_components_in_entity(entity) const;

		ecs_iterative_container decltype(auto) get_all_entities() const;

		template <typename... requires_t, typename... excludes_t>
		ecs_iterative_container decltype(auto) filter_entities(excludes_t...) const;

		template <typename... requires_t, typename... excludes_t>
		ecs_iterative_container decltype(auto) query_components(excludes_t...) const;

		template <ecs_system_callback ecs_system_callback_t>
		auto add_system(ecs_system_callback_t sys_fn);

		auto disable_system();

		bool is_valid(entity);

		world::detail impl;
	};

	struct WorldRegistry {
	private:
		struct HandleEntry {
			std::uint32_t m_Idx : 31;
			std::uint32_t m_Destroy : 1;
			
			HandleEntry() = default;
			HandleEntry(std::uint32_t id) : m_Idx{ id }, m_Destroy{} {}

			operator bool() const {
				return m_Destroy ^ 1;
			}
			operator std::uint32_t() const {
				return m_Idx;
			}
		};
		std::vector<std::pair<std::uint32_t, entt::registry>> m_packed_worlds;
		std::vector<HandleEntry> m_sparse_handles; //no paging, unlikely to have too many worlds
		std::uint32_t m_FreeList;

		WorldRegistry() : m_packed_worlds{}, m_sparse_handles{}, m_FreeList{null_handle31} {}; //pseudo singleton
		
	public:
		static WorldRegistry& Instance();
		static world NewWorld();
		static void EraseWorld(world);
		static void Clear();

		static bool Exists(world);
		
		entt::registry& operator[](int);

		~WorldRegistry() = default;
	};

	template<typename component_t, typename ...c_args>
	inline component_t& world::add_component_to_entity(entity enty, c_args&&... cargs)
	{
		assert(enty && "invalid entity");
		if (enty.all<component_t>()) {
			return enty.get<component_t>();
		}
		if constexpr (std::is_same_v<component_t, TransformComponent>) {
			impl.get_registry().emplace<TransformMtxComponent>(detail::entt_entity_cast(enty));
		}
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
		impl.get_registry().remove<component_ts...>(detail::entt_entity_cast(enty));
	}

	template<typename ...component_ts>
	inline bool world::has_any_components_in_entity(entity enty) const
	{
		return impl.get_registry().any_of<component_ts...>(detail::entt_entity_cast(enty));
	}

	template<typename ...component_ts>
	inline bool world::has_all_components_in_entity(entity enty) const
	{
		return impl.get_registry().all_of<component_ts...>(detail::entt_entity_cast(enty));
	}

	template <typename component_t, typename ...c_args>
	inline component_t& entity::add(c_args&&... cargs) 
	{
		return world(impl.descriptor).add_component_to_entity<component_t>(*this, std::forward<c_args>(cargs)...);
	}

	template <typename ...component_t>
	inline void entity::remove() 
	{
		world(impl.descriptor).remove_component_from_entity<component_t...>(*this);
	}

	template <typename ...component_t>
	inline decltype(auto) entity::get() const
	{
		return world(get_world_handle()).get_component_from_entity<component_t...>(*this);
	}

	template <typename ...requires_t>
	inline bool entity::all() const
	{
		return world(get_world_handle()).has_all_components_in_entity<requires_t...>(*this);
	}
	template <typename ...requires_t>
	inline bool entity::any() const
	{
		return world(get_world_handle()).has_any_components_in_entity<requires_t...>(*this);
	}
}
#endif