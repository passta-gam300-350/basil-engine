//#pragma once
//#include <iostream>
//#include <cassert>
//#include <unordered_map>
//#include <vector>
//#include "fmod.hpp"
//#include "fmod_errors.h"
//
//#include "../examples/common.h" // TEMP
//
//#define AUDIO_EXTENSION ".ogg"
//#define DOPPLERSCALE 1.0f
//#define DISTANCEFACTOR 1.0f
//#define ROLLOFFSCALE 1.0f
//#define MINDISTANCE 1.0f
//#define MAXDISTANCE 1000.0f
//
//typedef enum
//{
//	LOOP,
//	NO_LOOP
//}Sound_Mode;
//
//typedef enum
//{
//	LOW_PASS,
//	HIGH_PASS,
//	ECHO,
//	DISTORTION,
//	SIZE_OF_FILTERS
//}Filter;
//
//namespace Audio
//{
//#pragma region Helper Functions
//	void FMOD_ErrorCheck(FMOD_RESULT _result);
//#pragma endregion
//
//#pragma region Private Functions
//	void Play_Audio();
//	void Stop_Audio();
//	void Stop_All_Audio();
//	void Load_Audio(std::string _dir = "", bool _loop, bool _stream, bool _3d, bool _linear = false);
//	void Unload_Audio();
//	void Unload_All_Audio();
//	FMOD_MODE Mode_Selector(Sound_Mode _mode);
//	FMOD_DSP_TYPE Filter_Selector(Filter _filter);
//#pragma endregion
//
//#pragma region Public Functions
//	void Init(void* extradriverdata = nullptr);
//
//	void Update();
//
//	void Exit();
//#pragma endregion
//}