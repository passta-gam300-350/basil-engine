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
#include "spdlog/spdlog.h"
#include <rsc-core/guid.hpp>
#include <rsc-core/registry.hpp>
#include <native/audio.h>

// Forward declare audio resource type for ResourceRegistry
// Runtime type is int (sound handle), native type is AudioResourceData (metadata)
RegisterResourceTypeForward(int, "audio", audiodefine)

#define AUDIO_PATH "assets/audio/"
#define AUDIO_EXTENSION ".ogg"
#define DOPPLERSCALE 1.0f
#define DISTANCEFACTOR 1.0f
#define ROLLOFFSCALE 1.0f
#define MINDISTANCE 1.0f
#define MAXDISTANCE 1000.0f

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
//struct SoundHandle { uint32_t id{0};};
//
//struct AudioComponent
//{
//	enum class Sound_Mode : std::uint8_t
//	{
//		LOOP,
//		NO_LOOP
//	}m_Mode;
//	enum class Filter : std::uint8_t
//	{
//		LOW_PASS,
//		HIGH_PASS,
//		ECHO,
//		DISTORTION,
//		SIZE_OF_FILTERS
//	}m_Filter;
//	std::string m_SoundPath;
//	bool m_IsActive;
//	bool m_3D;
//	bool m_Linear;
//	bool m_PlayOnAwake;
//	bool m_Loop;
//	float m_Volume;
//	float m_MinDistance;
//	float m_MaxDistance;
//	std::string m_ChannelGroup;
//	std::vector<Filter> m_Filters;
//};
//
//struct AudioSystem : public ecs::SystemBase
//{
//public:
//	static FMOD_MODE Mode_Selector(Sound_Mode mode) noexcept;
//	static FMOD_DSP_TYPE Filter_Selector(Filter filter) noexcept;
//
//	static void FMOD_ErrorCheck(FMOD_RESULT result);
//	static FMOD_VECTOR Vec3_To_FMOD(const glm::vec3& v) noexcept;
//	static glm::vec3 FMOD_to_Vec3(const FMOD_VECTOR& v) noexcept;
//	static float dbToVolume(float dB) noexcept;
//	static float VolumeTodB(float volume) noexcept;
//
//	static void Play_Audio(const std::string& path, float volume = 1.0f, std::string group = "MASTER");
//	static void Stop_Audio();
//	static void Stop_All_Audio();
//	static void Load_Audio(std::string dir, bool stream, bool ambient, bool dimension, bool linear, bool playOnAwake, bool loop, float minDistance = MINDISTANCE, float maxDistance = MAXDISTANCE);
//	static void Unload_Audio();
//	static void Unload_All_Audio();
//
//	// Convenience functions for 3D audio with glm::vec3
//	static void Set3DListenerAttributes(const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& forward, const glm::vec3& up);
//	static void Set3DSoundAttributes(FMOD::Channel* channel, const glm::vec3& position, const glm::vec3& velocity);
//	static void RegisterChannel3DPosition(FMOD::Channel* channel, const glm::vec3& position, const glm::vec3& velocity = glm::vec3(0.0f));
//	static void UpdateAllChannel3DAttributes();
//private:
//	//static std::unique_ptr<InstanceData& InstancePtr();
//public:
//	static AudioSystem System() noexcept;
//	//~AudioSystem() = default;
//
//	void Init(void* extradriverdata = nullptr);
//	void Update(ecs::world&);
//	void Exit();
//};

// Forward declaration
struct AudioComponent;

inline FMOD_VECTOR ToFMOD(const glm::vec3& v) noexcept { return { v.x, v.y, v.z }; }
inline glm::vec3 ToVec3(const FMOD_VECTOR& v) noexcept { return { v.x, v.y, v.z }; }
inline void FMOD_ErrorCheck(FMOD_RESULT result) {
	if (result != FMOD_OK) {
        spdlog::warn("Audio: {}", FMOD_ErrorString(result));
		//assert(false && "FMOD Error encountered");
	}
}
inline float dbToVolume(float dB) noexcept { return powf(10.0f, 0.05f * dB); };
inline float VolumeTodB(float volume) noexcept { return 20.0f * log10f(volume); };

class AudioSystem : public ecs::SystemBase
{
    friend struct AudioComponent; // Allow AudioComponent to access internal channel map
public:
    static AudioSystem& GetInstance();

    bool Init(void* extraDriverData = nullptr);
    void Update(ecs::world& world);
    void Exit();

    void SetListenerPosition(const glm::vec3& position = glm::vec3(), const glm::vec3& velocity = glm::vec3()) noexcept;
    void SetListenerOrientation(const glm::vec3& forward = glm::vec3(), const glm::vec3& up = glm::vec3()) noexcept;

    // Asset management
    int LoadSound(const std::string& filePath, bool is3D = true, bool isStream = false, bool isLooping = false);
    void UnloadSound(int soundHandle);

    // Low-level access
    FMOD::System* GetSystem() const;
    FMOD::Sound* GetSound(int soundHandle) const;
    FMOD::Channel* GetChannel(AudioComponent* component) const;

    // Component registration
    void RegisterComponent(AudioComponent* component);
    void UnregisterComponent(AudioComponent* component);

    // State
    bool IsInitialized() const;

    // Non-copyable
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

private:
    // Private constructor (singleton)
    AudioSystem();

    // Member variables - all state encapsulated in class
    FMOD::System* m_system;
    FMOD_RESULT m_result;
    bool m_initialized;
    int m_nextSoundHandle;
    std::unordered_map<int, FMOD::Sound*> m_loadedSounds;
    std::vector<AudioComponent*> m_components;
	std::unordered_map<std::string, int> m_pathToHandle;
	std::unordered_map<int, int> m_refCounts;

    // Channel management (moved from AudioComponent)
    std::unordered_map<AudioComponent*, FMOD::Channel*> m_componentChannels;

    // Listener state
    glm::vec3 m_listenerPosition;
    glm::vec3 m_listenerVelocity;
    glm::vec3 m_listenerForward;
    glm::vec3 m_listenerUp;
    glm::vec3 m_listenerLastPosition;
    bool m_listenerMoved;
};

// 3D Audio Component - attach to game entities (pure data, no FMOD pointers)
struct AudioComponent
{
    // Asset reference (GUID-based system only)
    rp::BasicIndexedGuid audioAssetGuid{ rp::null_guid, 0 };

    // Internal handle (managed by AudioSystem)
    int soundHandle = -1;

    // 3D positioning
    glm::vec3 position{ 0.0f, 0.0f, 0.0f };
    glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
    float minDistance = MINDISTANCE;
    float maxDistance = MAXDISTANCE;

    // Audio properties
    float volume = 1.0f;
    bool isLooping = false;
    bool is3D = true;
    bool isStreaming = false;
    bool playOnAwake = false;

    // Playback state (updated by AudioSystem)
    bool isPlaying = false;
    bool isPaused = false;
    bool isInitialized = false;

    // Playback info (updated each frame)
    float playbackPosition = 0.0f; // in seconds
    float duration = 0.0f; // in seconds

    // Constructor
    AudioComponent() = default;
    AudioComponent(int soundHandle, const glm::vec3& pos = glm::vec3())
        : soundHandle(soundHandle), position(pos) {}
    ~AudioComponent();

    // Initialize component from loaded sound handle (refreshes metadata)
    void RefreshSoundInfo();

    void UpdatePosition(const glm::vec3& newPosition);
    void UpdateVelocity(const glm::vec3& newVelocity);

    bool Play();
    bool Pause();
    bool Resume();
    bool Stop();

    void SetVolume(float vol);
    void SetDistanceRange(float minDist, float maxDist);

    bool IsPlaying() const;

    void InternalUpdate();
};
#endif
