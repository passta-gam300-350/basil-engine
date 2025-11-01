#ifndef LIB_ECS_RANGES
#define LIB_ECS_RANGES

#include "ecs/fwd.h"
#include "ecs/internal/entity.h"

namespace ecs {
	//not thread safe
	template<typename T>
	concept HasSizeHint = requires(T t) {
		{ t.size_hint() } -> std::convertible_to<std::size_t>;
	};

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
			pointer operator->() {
				ref_physical_address = entity{ world_handle, static_cast<std::uint32_t>(*it) };
				return &ref_physical_address;
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
			if constexpr (HasSizeHint<range_based_container_t>) {
				return entities.size_hint();
			}
			else {
				return entities.size();
			}
		}

		//some overhead
		std::size_t size() {
			if constexpr (HasSizeHint<range_based_container_t>) {
				std::size_t sz{};
				for (auto i = entities.begin(); i != entities.end(); i++) {
					sz++;
				}
				return sz;
			}
			else {
				return entities.size();
			}
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

	template <typename Type>
	struct iterator_temp_storage {
		iterator_temp_storage(Type&& tval) : value{ std::move(tval) } {};

		Type& operator*() {
			return value;
		}
		Type* operator->() {
			return &value;
		}

		Type value;
	};

	// Primary template: false
	template <typename, typename = void>
	struct is_tuple_like : std::false_type {};

	// Specialization: true if std::tuple_size<T> is well-formed
	template <typename T>
	struct is_tuple_like<T, std::void_t<decltype(std::tuple_size<T>::value)>>
		: std::true_type {
	};

	template <typename T>
	inline constexpr bool is_tuple_like_v = is_tuple_like<T>::value;


	template <ecs_iterative_container range_based_container_t, typename ...includes>
	struct component_range {
		template <typename cont_iterator>
		struct iterator {
			cont_iterator it;
			
			iterator(cont_iterator&& iter) : it{ std::forward<cont_iterator>(iter) } {}

			auto dereference_iter() {
				return *it;
			}

			template <typename Tuple, std::size_t...Is>
			auto repack_tuple(Tuple&& tp, std::index_sequence<Is...>) {
				return std::forward_as_tuple(std::get<Is + 1>(std::forward<Tuple>(tp))...);
			}

			auto operator*() {
				auto value{ dereference_iter() };
				static_assert(is_tuple_like_v<decltype(value)>);
				constexpr std::size_t t_size{ std::tuple_size<decltype(value)>::value };
				return repack_tuple(value, std::make_index_sequence<t_size - 1>{});
			}
			iterator operator++() {
				it++;
				return *this;
			}
			iterator operator--() {
				it--;
				return *this;
			}
			bool operator!=(iterator rhs) {
				return it != rhs.it;
			}
			bool operator==(iterator rhs) {
				return it == rhs.it;
			}
			auto operator->() {
				return iterator_temp_storage(operator*());
			}
		};

		component_range() = delete;
		component_range(range_based_container_t const& cont, include_t<includes...>) : components{ cont } {}
		component_range(range_based_container_t&& cont, include_t<includes...>) noexcept : components{ std::forward<range_based_container_t>(cont) } {}
		component_range(component_range const& er) : components(er.components) {}
		component_range(component_range&& er) noexcept : components(std::move(er.components)) {}

		auto begin() {
			return iterator(components.each().begin());
		}
		auto end() {
			return iterator(components.each().end());
		}

		//this is not accurate bte, its an estimate
		std::size_t size_hint() {
			return components.size_hint();
		}

		//some overhead
		std::size_t size() {
			std::size_t sz{};
			for (auto i = components.begin(); i != components.end(); i++) {
				sz++;
			}
			return sz;
		}

		decltype(auto) front() {
			return *begin();
		}

		bool empty() {
			return components.begin() == components.end();
		}

		operator bool() {
			return components.begin() != components.end();
		}

		range_based_container_t components;
	};
}

#endif