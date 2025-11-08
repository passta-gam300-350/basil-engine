#ifndef ManagedAudio_HPP
#define ManagedAudio_HPP
#include <cstdint>

class ManagedAudio
{
public:
	static void SetVolume(uint64_t handle, float volume);
	static void SetIsLooping(uint64_t handle, bool isLooping);

	static float GetVolume(uint64_t handle);
	static float GetIsLooping(uint64_t handle);

	static void Play(uint64_t handle);
	static void Pause(uint64_t handle);
	static void Stop(uint64_t handle);
};

#endif //!AUDIOBINDING_HPP