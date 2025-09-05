#ifndef LIB_ECS_ENTITY_H
#define LIB_ECS_ENTITY_H

#include <cstdint>

namespace ecs {
	struct entity {
		using entity_id_t = uint32_t;

		union {
			struct {
				std::uint64_t descriptor : 32; //mainly used for world
				std::uint64_t id : 32;
			};
			std::uint64_t handle;
		};

		operator entity_id_t() const {
			return id;
		};
	};
	/*
	template <ecs_iterative_container range_based_container_t>
	struct entity_range {

		struct iterator {
			using value_type = entity;
			using pointer = entity*;
			using reference = entity&;

			iterator() = delete;
			iterator(std::uint32_t wid, range_based_container_t::iterator rb_it) : world_handle{ wid }, it{ rb_it } {}
			iterator(iterator const& iter) : world_handle{ iter.world_handle }, it{ iter.it } {}

			value_type operator*() {
				return entity{ world_handle, static_cast<std::uint32_t>(*it) };
			}
			iterator operator++() {
				return iterator{ world_handle, it++ };
			}
			iterator operator--() {
				return iterator{ world_handle, it-- };
			}
			bool operator!=(iterator rhs) {
				return it!=rhs.it;
			}

		private:
			std::uint32_t world_handle;
			range_based_container_t::iterator it;
		};

		entity_range() = delete;
		entity_range(std::uint32_t world_id, range_based_container_t const& cont) : world_handle{ world_id }, entities{ cont } {}
		entity_range(std::uint32_t world_id, range_based_container_t&& cont) noexcept : world_handle{ world_id }, entities{ std::forward<range_based_container_t>(cont) } {}
		entity_range(entity_range const& er) : world_handle{ er.world_handle }, entities(er.entities) {}
		entity_range(entity_range&& er) noexcept : world_handle{ er.world_handle }, entities(std::move(er.entities)) {}

		iterator begin();
		iterator end();

		std::uint32_t world_handle;
		range_based_container_t entities;
	};
	*/
}

#endif