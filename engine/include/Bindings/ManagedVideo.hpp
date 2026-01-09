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
