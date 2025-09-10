#ifndef LIB_ECS_FWD_H
#define LIB_ECS_FWD_H

#include <ranges>
#include <string_view>
#include <functional>

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

	struct Scheduler;
	struct system_registry;
	struct generic_system_wrapper;

	using system_callback = std::function<void(world)>;

	template <typename ecs_system_callback_t>
	concept ecs_system_callback = requires (ecs_system_callback_t sys_cb) {
		{ sys_cb } -> std::convertible_to<system_callback>;
	};

	constexpr std::uint32_t null_handle32{ ~(std::uint32_t(0x0)) };
	constexpr std::uint64_t null_handle64{ ~(std::uint64_t(0x0))};
	constexpr std::string_view default_name{"unnamed"};
}

#endif