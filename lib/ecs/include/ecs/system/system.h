#ifndef LIB_ECS_SYS_H
#define LIB_ECS_SYS_H

#include "ecs/fwd.h"
#include <functional>

namespace ecs {
	struct generic_system_wrapper {
	public:
		void invoke();
		template <typename func>
		generic_system_wrapper(func cb) : m_callback{cb} {}
		system_callback get() { return m_callback; }

	private:
		system_callback m_callback;
	};

	struct system_registry {
		std::vector<std::function<void(entt::registry&)>> system_callbacks;
	};
}

#endif