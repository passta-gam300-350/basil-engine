#ifndef LIB_ECS_QUERY_H
#define LIB_ECS_QUERY_H

namespace ecs {
	template <typename... requires_t, typename... excludes_t>
	inline ecs_iterative_container decltype(auto) world::filter_entities(excludes_t... ex) const
	{
		return entity_range(impl.handle, impl.get_registry().view<requires_t...>(ex...));
	}
}

#endif