/******************************************************************************/
/*!
\file   ManagedAudio.hpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the declaration for the ManagedAudio class, which
is responsible for handling audio playback and control from managed code (C#).


Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/


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