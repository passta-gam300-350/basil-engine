/******************************************************************************/
/*!
\file   ManagedAudio.cpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the implementation for the ManagedAudio class, which
is responsible for handling audio playback and control from managed code (C#).


Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/


#include "Bindings/ManagedAudio.hpp"

#include "ecs/internal/entity.h"
#include "System/Audio.hpp"

void ManagedAudio::SetIsLooping(uint64_t handle, bool isLooping)
{
	ecs::entity entity{ handle };
	auto& audio = entity.get<AudioComponent>();

	audio.isLooping = isLooping;
}

float ManagedAudio::GetIsLooping(uint64_t handle)
{
	ecs::entity entity{ handle };
	auto& audio = entity.get<AudioComponent>();
	return audio.isLooping;
}


void ManagedAudio::SetVolume(uint64_t handle, float volume)
{
	ecs::entity entity{ handle };
	auto& audio = entity.get<AudioComponent>();

	audio.SetVolume(volume);
}

float ManagedAudio::GetVolume(uint64_t handle)
{
	ecs::entity entity{ handle };
	auto& audio = entity.get<AudioComponent>();
	return audio.volume;
}

void ManagedAudio::Play(uint64_t handle)
{
	ecs::entity entity{ handle };
	auto& audio = entity.get<AudioComponent>();

	audio.Play();
}

void ManagedAudio::Pause(uint64_t handle)
{
	ecs::entity entity{ handle };
	auto& audio = entity.get<AudioComponent>();

	audio.Pause();
}

void ManagedAudio::Stop(uint64_t handle)
{
	ecs::entity entity{ handle };
	auto& audio = entity.get<AudioComponent>();

	audio.Stop();
}






