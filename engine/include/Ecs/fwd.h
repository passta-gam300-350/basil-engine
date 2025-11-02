#ifndef LIB_ECS_FWD_H
#define LIB_ECS_FWD_H

#include <ranges>
#include <string_view>
#include <functional>
#include <entt/entt.hpp>

namespace ecs {

	template <typename ecs_iterative_container_t>
	concept ecs_iterative_container = requires (ecs_iterative_container_t cont) {
		std::ranges::range<ecs_iterative_container_t>;
		//{ cont | cont } -> std::ranges::range;
	};

	struct entity;
	struct world;
	template <ecs_iterative_container cont>
	struct entity_range;

	template <typename ...types>
	struct type_set {
		using type = type_set;
		static constexpr std::uint64_t elem_ct{ sizeof...(types) };
	};

	template <typename ...excludes>
	struct exclude_t : type_set<excludes...> {
		exclude_t() = default;
	};

	template <typename ...includes>
	struct include_t : type_set<includes...> {
		include_t() = default;
	};
	/*
	template <typename ...excludes_t>
	inline constexpr exclude_t<excludes_t...> exclude{};

	template <typename ...excludes_t>
	inline constexpr exclude_t<excludes_t...> include{};
	*/

	template <typename ...excludes_t>
	using exclude_set = entt::exclude_t<excludes_t...>;

	template <typename ...excludes_t>
	inline constexpr exclude_set<excludes_t...> exclude{ entt::exclude<excludes_t...> };

	struct Scheduler;
	struct system_registry;
	struct generic_system_wrapper;

	using system_callback = std::function<void(world)>;

	template <typename ecs_system_callback_t>
	concept ecs_system_callback = requires (ecs_system_callback_t sys_cb) {
		{ sys_cb } -> std::convertible_to<system_callback>;
	};

	constexpr std::uint32_t null_handle31{ (~(std::uint32_t(0x0))>>1) };
	constexpr std::uint32_t null_handle32{ ~(std::uint32_t(0x0)) };
	constexpr std::uint64_t null_handle64{ ~(std::uint64_t(0x0))};
	constexpr std::string_view default_name{"unnamed"};
}

#endif