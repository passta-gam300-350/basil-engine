#ifndef LIB_ECS_RANGES
#define LIB_ECS_RANGES

#include "ecs/fwd.h"
#include "ecs/internal/entity.h"

namespace ecs {
	//not thread safe
	template <ecs_iterative_container range_based_container_t>
	struct entity_range {

		struct iterator {
			using value_type = entity;
			using pointer = entity*;
			using reference = entity&;

			iterator() = delete;
			iterator(std::uint32_t wid, range_based_container_t::iterator rb_it) : world_handle{ wid }, ref_physical_address{}, it{ rb_it } {}
			iterator(iterator const& iter) : world_handle{ iter.world_handle }, ref_physical_address{}, it{ iter.it } {}

			reference operator*() {
				return ref_physical_address = entity{ world_handle, static_cast<std::uint32_t>(*it) };
			}
			iterator operator++() {
				return iterator{ world_handle, it++ };
			}
			iterator operator--() {
				return iterator{ world_handle, it-- };
			}
			bool operator!=(iterator rhs) {
				return it != rhs.it;
			}
			bool operator==(iterator rhs) {
				return it == rhs.it;
			}

		private:
			std::uint32_t world_handle;
			entity ref_physical_address;
			range_based_container_t::iterator it;
		};

		entity_range() = delete;
		entity_range(std::uint32_t world_id, range_based_container_t const& cont) : world_handle{ world_id }, entities{ cont } {}
		entity_range(std::uint32_t world_id, range_based_container_t&& cont) noexcept : world_handle{ world_id }, entities{ std::forward<range_based_container_t>(cont) } {}
		entity_range(entity_range const& er) : world_handle{ er.world_handle }, entities(er.entities) {}
		entity_range(entity_range&& er) noexcept : world_handle{ er.world_handle }, entities(std::move(er.entities)) {}

		iterator begin() {
			return iterator(world_handle, entities.begin());
		}
		iterator end() {
			return iterator(world_handle, entities.end());
		}

		//this is not accurate bte, its an estimate
		std::size_t size_hint() {
			return entities.size_hint();
		}

		//some overhead
		std::size_t size() {
			std::size_t sz{};
			for (auto i = entities.begin(); i != entities.end(); i++) {
				sz++;
			}
			return sz;
		}

		entity front() {
			return *begin();
		}

		entity_range operator|(entity_range const& rhs) {
			assert(rhs.world_handle == world_handle && "disjoint set of entities. entities are not from the same world!");
			return entity_range(world_handle, rhs.entities | entities);
		}

		bool empty() {
			return entities.begin() == entities.end();
		}

		operator bool() {
			return entities.begin()!=entities.end();
		}

		std::uint32_t world_handle;
		range_based_container_t entities;
	};
}

#endif