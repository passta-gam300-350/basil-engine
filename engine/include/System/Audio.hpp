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

//#include "../examples/common.h" // [TEMP]

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
	std::string m_SoundName;
	bool m_IsActive;
	bool m_PlayOnAwake;
	bool m_3D;
	bool m_Loop;
	float m_Volume;
	//int m_Channel;
	std::vector<Filter> m_Filters;
};

struct AudioSystem : public ecs::SystemBase
{
public:
	static void FMOD_ErrorCheck(FMOD_RESULT _result);

	static void Play_Audio();
	static void Stop_Audio();
	static void Stop_All_Audio();
	static void Load_Audio(std::string _dir = "", bool _loop, bool _stream, bool _3d, bool _linear = false);
	static void Unload_Audio();
	static void Unload_All_Audio();

	static FMOD_MODE Mode_Selector(Sound_Mode _mode);
	static FMOD_DSP_TYPE Filter_Selector(Filter _filter);
private:
	//static std::unique_ptr<InstanceData& InstancePtr();
public:
	//static InstanceData& Instance();
	//static AudioSystem System();
	//~AudioSystem() = default;

	void Init(void* extradriverdata = nullptr);
	void Update();
	void Exit();
};
#endif