/******************************************************************************/
/*!
\file   query.h
\author Team PASSTA
		Chew Bangxin Steven (banxginsteven.chew@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/09
\brief    Declares the query ecs functions

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "world.h"
#ifndef LIB_ECS_QUERY_H
#define LIB_ECS_QUERY_H

namespace ecs {
	inline ecs_iterative_container decltype(auto) world::get_all_entities() const
	{
		return entity_range(impl.handle, impl.get_registry().view<entt::entity>());
	}
	template <typename... requires_t, typename... excludes_t>
	inline ecs_iterative_container decltype(auto) world::filter_entities(excludes_t... ex) const
	{
		return entity_range(impl.handle, impl.get_registry().view<entity::active_t, requires_t...>(ex...));
	}
	template <typename... requires_t, typename... excludes_t>
	inline ecs_iterative_container decltype(auto) ecs::world::query_components(excludes_t ... ex) const
	{
		return component_range(impl.get_registry().view<entity::active_t, requires_t...>(ex...), include_t<requires_t...>{});
	}
}

#endif