#ifndef ENGINE_AUDIO_H
#define ENGINE_AUDIO_H
#pragma once
#include <iostream>
#include <cassert>
#include <unordered_map>
#include <vector>
#include "../../vendor/fmod/api/core/inc/fmod.hpp"
#include "../../vendor/fmod/api/core/inc/fmod_errors.h"
#include "Ecs/ecs.h"
#include "glm/glm.hpp"

//#include "../examples/common.h" // [TEMP]

#define PATH "../test/examples/lib/resource/assets/audio/"
#define AUDIO_EXTENSION ".ogg"
#define DOPPLERSCALE 1.0f
#define DISTANCEFACTOR 1.0f
#define ROLLOFFSCALE 1.0f
#define MINDISTANCE 1.0f
#define MAXDISTANCE 1000.0f

typedef enum
{
	LOOP,
	NO_LOOP
}Sound_Mode;

typedef enum
{
	LOW_PASS,
	HIGH_PASS,
	ECHO,
	DISTORTION,
	SIZE_OF_FILTERS
}Filter;

struct SoundHandle { uint32_t id{0};};

struct AudioComponent
{
	enum class Sound_Mode : std::uint8_t
	{
		LOOP,
		NO_LOOP
	}m_Mode;
	enum class Filter : std::uint8_t
	{
		LOW_PASS,
		HIGH_PASS,
		ECHO,
		DISTORTION,
		SIZE_OF_FILTERS
	}m_Filter;
	std::string m_SoundPath;
	bool m_IsActive;
	bool m_3D;
	bool m_Linear;
	bool m_PlayOnAwake;
	bool m_Loop;
	float m_Volume;
	float m_MinDistance;
	float m_MaxDistance;
	std::string m_ChannelGroup;
	std::vector<Filter> m_Filters;
};

struct AudioSystem : public ecs::SystemBase
{
public:
	static FMOD_MODE Mode_Selector(Sound_Mode mode) noexcept;
	static FMOD_DSP_TYPE Filter_Selector(Filter filter) noexcept;

	static void FMOD_ErrorCheck(FMOD_RESULT result);
	static FMOD_VECTOR Vec3_To_FMOD(const glm::vec3& v) noexcept;
	static glm::vec3 FMOD_to_Vec3(const FMOD_VECTOR& v) noexcept;
	static float dbToVolume(float dB) noexcept;
	static float VolumeTodB(float volume) noexcept;

	static void Play_Audio(const std::string& path, float volume = 1.0f, std::string group = "MASTER");
	static void Stop_Audio();
	static void Stop_All_Audio();
	static void Load_Audio(std::string dir, bool stream, bool ambient, bool dimension, bool linear, bool playOnAwake, bool loop, float minDistance = MINDISTANCE, float maxDistance = MAXDISTANCE);
	static void Unload_Audio();
	static void Unload_All_Audio();

	// Convenience functions for 3D audio with glm::vec3
	static void Set3DListenerAttributes(const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& forward, const glm::vec3& up);
	static void Set3DSoundAttributes(FMOD::Channel* channel, const glm::vec3& position, const glm::vec3& velocity);
	static void RegisterChannel3DPosition(FMOD::Channel* channel, const glm::vec3& position, const glm::vec3& velocity = glm::vec3(0.0f));
	static void UpdateAllChannel3DAttributes();
private:
	//static std::unique_ptr<InstanceData& InstancePtr();
public:
	static AudioSystem System() noexcept;
	//~AudioSystem() = default;

	void Init(void* extradriverdata = nullptr);
	void Update(ecs::world&);
	void Exit();
};
#endif