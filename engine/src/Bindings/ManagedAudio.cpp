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






