#ifndef LIB_ECS_FWD_H
#define LIB_ECS_FWD_H

namespace ecs {

	template <typename ecs_iterative_container_t>
	concept ecs_iterative_container = requires (ecs_iterative_container_t cont) {
		std::ranges::range<ecs_iterative_container_t>;
		{ cont | cont } -> std::ranges::range;
	};

	struct entity;
	struct world;
	template <ecs_iterative_container cont>
	struct entity_range;

}

#endif