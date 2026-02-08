/******************************************************************************/
/*!
\file   ManagedVideo.hpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3451 / UXG3450
\date   2026/02/01
\brief This file contains the declaration for the ManagedVideo class, which
is responsible for managing video components from managed code (C#).


Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MANAGED_VIDEO_HPP
#define MANAGED_VIDEO_HPP
#include <cstdint>
#include "ecs/ecs.h"
#include "Render/VideoComponent.hpp"

class ManagedVideo
{
public:
	static bool GetIsPlaying(uint64_t handle)
	{
		ecs::entity ent{ handle };
		VideoComponent& cmp = ent.get<VideoComponent>();
		return cmp.isPlaying;
	}

	static void SetIsLooping(uint64_t handle, mono_bool isLooping)
	{
		ecs::entity ent{ handle };
		VideoComponent& cmp = ent.get<VideoComponent>();
		cmp.loop = isLooping;
	}

	static void SetPlaying(uint64_t handle, mono_bool isPlaying)
	{
		ecs::entity ent{ handle };
		VideoComponent& cmp = ent.get<VideoComponent>();
		cmp.isPlaying = isPlaying;
	}
};

#endif // !MANAGED_VIDEO_HPP
