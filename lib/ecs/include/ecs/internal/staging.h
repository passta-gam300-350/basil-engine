#ifndef LIB_ECS_STAGING_H
#define LIB_ECS_STAGING_H

namespace ecs {
	enum class stages : std::uint32_t {
		on_run,
		on_load,
		pre_update,
		on_update,
		post_update,
		on_exit
	};
}

#endif