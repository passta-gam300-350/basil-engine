/******************************************************************************/
/*!
\file   staging.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  ECS staging definitions

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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