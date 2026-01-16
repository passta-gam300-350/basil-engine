/******************************************************************************/
/*!
\file   Audio.hpp
\author Team PASSTA
        Halis Ilyasa Bin Amat Sarijan (halisilyasa.b@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares AudioSystem and audio component to play sound in engine.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

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

// Forward declaration
struct AudioComponent;

// Helper functions
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

    // System states
    bool Init(void* extraDriverData = nullptr);
    void Update(ecs::world& world);
    void Exit();

    // Set position and orientation for audio listeners (i.e. camera)
    void SetListenerPosition(const glm::vec3& position = glm::vec3(), const glm::vec3& velocity = glm::vec3()) noexcept;
    void SetListenerOrientation(const glm::vec3& forward = glm::vec3(), const glm::vec3& up = glm::vec3()) noexcept;

    // Asset management of sound files
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
    rp::BasicIndexedGuid audioAssetGuid{ static_cast<rp::BasicIndexedGuid>(rp::TypeNameGuid<"audio">{}) };

    // Internal handle (managed by AudioSystem)
    int soundHandle = -1;

    // 3D positioning
    glm::vec3 position{ 0.0f, 0.0f, 0.0f };
    glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
    float minDistance = MINDISTANCE;
    float maxDistance = MAXDISTANCE;

    // Audio properties
    // [NEW]
    //FMOD::Group* group = set to master;
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

    // Sound initializers for component
    bool Init(int handle);
    bool Init(const std::string& filePath, bool is3D = true, bool isStream = false, bool isLooping = false);

    // Updates position and velocity of audio emitter
    void UpdatePosition(const glm::vec3& newPosition);
    void UpdateVelocity(const glm::vec3& newVelocity);

    // Playback controls
    bool Play();
    bool Pause();
    bool Resume();
    bool Stop();

    // Sets audio parameters
    void SetVolume(float vol);
    void SetLoop(bool loop);
    void SetDistanceRange(float minDist, float maxDist);
    void RefreshSoundInfo();

    // Checks if sound is playing
    bool IsPlaying() const;

    // Carries sound updates into AudioSystem::Update()
    void InternalUpdate();
};
#endif
