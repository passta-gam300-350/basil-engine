/******************************************************************************/
/*!
\file   Audio.cpp
\author Team PASSTA
        Halis Ilyasa Bin Amat Sarijan (halisilyasa.b@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Defines AudioSystem and audio component to play sound in engine.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#include "System/Audio.hpp"
#include "Component/Transform.hpp"
#include "Engine.hpp"
#include <rsc-core/serialization/serializer.hpp>
#include "Manager/ResourceSystem.hpp"
#include "Render/Camera.h"
#include <filesystem>
#include <algorithm>
#include "Profiler/profiler.hpp"

namespace {

static FMOD_DSP_TYPE ToFMODDSPType(AudioFilterType t) {
    switch (t) {
        case AudioFilterType::Lowpass: return FMOD_DSP_TYPE_LOWPASS;
        case AudioFilterType::Highpass: return FMOD_DSP_TYPE_HIGHPASS;
        case AudioFilterType::Echo: return FMOD_DSP_TYPE_ECHO;
        default: return FMOD_DSP_TYPE_UNKNOWN;
    }
}

static void SyncFilterParams(FMOD::DSP* dsp, AudioFilterType type, const AudioFilterParams& params) {
    switch (type) {
        case AudioFilterType::Lowpass: {
            float cutoff = std::clamp(params.cutoffHz, 10.0f, 22050.0f);
            FMOD_ErrorCheck(dsp->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, cutoff));
            FMOD_ErrorCheck(dsp->setParameterFloat(FMOD_DSP_LOWPASS_RESONANCE, std::clamp(params.resonance, 0.5f, 10.0f)));
            break;
        }
        case AudioFilterType::Highpass: {
            float cutoff = std::clamp(params.cutoffHz, 10.0f, 22050.0f);
            FMOD_ErrorCheck(dsp->setParameterFloat(FMOD_DSP_HIGHPASS_CUTOFF, cutoff));
            FMOD_ErrorCheck(dsp->setParameterFloat(FMOD_DSP_HIGHPASS_RESONANCE, std::clamp(params.resonance, 0.5f, 10.0f)));
            break;
        }
        case AudioFilterType::Echo: {
            float delayMs = std::clamp(params.echoDelayMs, 1.0f, 5000.0f);
            float feedback = std::clamp(params.echoFeedback, 0.0f, 1.0f);
            FMOD_ErrorCheck(dsp->setParameterFloat(FMOD_DSP_ECHO_DELAY, delayMs));
            FMOD_ErrorCheck(dsp->setParameterFloat(FMOD_DSP_ECHO_FEEDBACK, feedback));
            FMOD_ErrorCheck(dsp->setParameterFloat(FMOD_DSP_ECHO_DRYLEVEL, 1.0f));
            FMOD_ErrorCheck(dsp->setParameterFloat(FMOD_DSP_ECHO_WETLEVEL, 0.5f));
            break;
        }
        default:
            break;
    }
}

// Attempts to load the FMOD sound handle from the serialized audio GUID.
// Needed because scenes only serialize the GUID, not the runtime soundHandle.
bool EnsureSoundLoaded(AudioComponent& audio) {
    if (audio.isInitialized) {
        return true;
    }

    if (audio.soundHandle < 0) {
        // No sound loaded yet – fetch it from the resource registry using the GUID.
        if (audio.audioAssetGuid.m_guid == rp::null_guid) {
            spdlog::warn("AudioComponent: Cannot initialize without an audio GUID");
            return false;
        }

        ResourceRegistry::Entry* entry = ResourceRegistry::Instance().Pool(audio.audioAssetGuid);
        if (!entry) {
            spdlog::warn("AudioComponent: No resource pool found for audio type {}", audio.audioAssetGuid.m_typeindex);
            return false;
        }

        ResourceHandle handle = entry->m_Vt.m_Get(entry->m_Pool, audio.audioAssetGuid.m_guid);
        int* soundHandlePtr = static_cast<int*>(entry->m_Vt.m_Ptr(entry->m_Pool, handle));

        if (!soundHandlePtr || *soundHandlePtr < 0) {
            spdlog::warn("AudioComponent: Failed to resolve sound handle for GUID {}", audio.audioAssetGuid.m_guid.to_hex());
            return false;
        }

        audio.soundHandle = *soundHandlePtr;
        audio.isInitialized = false;
    }

    audio.RefreshSoundInfo();
    return audio.isInitialized;
}

} // namespace

AudioSystem& AudioSystem::GetInstance() {
    static AudioSystem instance;
    return instance;
}

AudioSystem::AudioSystem()
    : m_system(nullptr)
    , m_result(FMOD_OK)
    , m_initialized(false)
    , m_nextSoundHandle(1)
    , m_listenerPosition(0.0f, 0.0f, 0.0f)
    , m_listenerVelocity(0.0f, 0.0f, 0.0f)
    , m_listenerForward(0.0f, 0.0f, 1.0f)
    , m_listenerUp(0.0f, 1.0f, 0.0f)
    , m_listenerLastPosition(0.0f, 0.0f, 0.0f)
    , m_listenerMoved(false)
{}

bool AudioSystem::Init(void* extraDriverData) {
    if (m_initialized)
        return true;

    spdlog::info("Audio: Creating system");
    FMOD_ErrorCheck(FMOD::System_Create(&m_system));

    spdlog::info("Audio: Initializing system");
    FMOD_ErrorCheck(m_system->init(512, FMOD_INIT_NORMAL, extraDriverData));


    spdlog::info("Audio: Setting 3D parameters");
    FMOD_ErrorCheck(m_system->set3DSettings(DOPPLERSCALE, DISTANCEFACTOR, ROLLOFFSCALE)); //TEMP (Set saved 3D settings)

    // Setup channel groups (MASTER/BGM/SFX/UI/AMBIENT)
    m_masterGroup = nullptr;
    m_groups.clear();
    FMOD_ErrorCheck(m_system->getMasterChannelGroup(&m_masterGroup));
    if (m_masterGroup) {
        auto makeGroup = [&](AudioGroup g, const char* name) {
            FMOD::ChannelGroup* cg = nullptr;
            FMOD_ErrorCheck(m_system->createChannelGroup(name, &cg));
            if (cg) {
                // Route into master for global volume/mute control
                FMOD_ErrorCheck(m_masterGroup->addGroup(cg));
                m_groups[g] = cg;
            }
        };
        makeGroup(AudioGroup::BGM, "BGM");
        makeGroup(AudioGroup::SFX, "SFX");
        makeGroup(AudioGroup::UI, "UI");
        makeGroup(AudioGroup::AMBIENT, "AMBIENT");
    } else {
        spdlog::warn("Audio: Failed to acquire master channel group; audio groups disabled");
    }

    m_listenerPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    m_listenerVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
    m_listenerLastPosition = m_listenerPosition;
    m_listenerMoved = false;

    auto e = Engine::GetWorld().add_entity();
    e.add<AudioComponent>();
    e.destroy();

    m_initialized = true;
    return true;
}

void AudioSystem::Update(ecs::world& world) {
    if (!m_initialized || !m_system)
        return;
    PF_SYSTEM("Audio System");
    // Update listener from active game camera (cached @ 60Hz in CameraSystem::FixedUpdate)
    // Editor camera is separate and not used for audio positioning
    const auto& camera = CameraSystem::GetActiveCameraData();
    if (!camera.isActive) {
        // No active camera in scene - skip audio update this frame
        FMOD_ErrorCheck(m_system->update());
        return;
    }
    SetListenerPosition(camera.position);
    SetListenerOrientation(camera.front, camera.up);

    // Update listener velocity and position
    if (m_listenerMoved) {
        constexpr float deltaTime = 1.0f / 60.0f;
        m_listenerVelocity.x = (m_listenerPosition.x - m_listenerLastPosition.x) / deltaTime;
        m_listenerVelocity.y = (m_listenerPosition.y - m_listenerLastPosition.y) / deltaTime;
        m_listenerVelocity.z = (m_listenerPosition.z - m_listenerLastPosition.z) / deltaTime;
        m_listenerLastPosition = m_listenerPosition;
        m_listenerMoved = false;
    }

    const FMOD_VECTOR pos = ToFMOD(m_listenerPosition);
    const FMOD_VECTOR vel = ToFMOD(m_listenerVelocity);
    const FMOD_VECTOR forward = ToFMOD(m_listenerForward);
    const FMOD_VECTOR up = ToFMOD(m_listenerUp);

    FMOD_ErrorCheck(m_system->set3DListenerAttributes(0, &pos, &vel, &forward, &up));

    // Auto-sync AudioComponent positions with TransformComponent
    auto audioEntities = world.filter_entities<AudioComponent, TransformComponent>();
    for (auto entity : audioEntities) {
        auto [audio, transform] = entity.get<AudioComponent, TransformComponent>();

        // Update AudioComponent position from Transform
        glm::vec3 worldPos = transform.m_Translation;
        if (audio.position != worldPos) {
            audio.UpdatePosition(worldPos);
        }
    }

    // Update all audio components
    for (auto* component : m_components)
        if (component)
            component->InternalUpdate();

    FMOD_ErrorCheck(m_system->update());
}

void AudioSystem::StopAll() {
    if (!m_initialized || !m_system || !m_masterGroup)
        return;

    spdlog::info("Audio: Stopping all playback");
    // Release filter DSPs (need channel to remove DSP from)
    for (auto& pair : m_componentChannels) {
        if (pair.first && pair.second)
            RemoveFilterDsp(pair.first, pair.second);
    }
    m_componentFilterDsp.clear();
    // Stop every channel in the mix (component, video, BGM, dialogue, etc.)
    FMOD_ErrorCheck(m_masterGroup->stop());
    m_componentChannels.clear();
}

void AudioSystem::Exit() {
    spdlog::info("Audio: Exiting");
    if (!m_initialized)  return;

    StopAll();

    spdlog::info("Audio: Unregistering audio components");
    m_components.clear();

    spdlog::info("Audio: Releasing sounds");
    for (auto& pair : m_loadedSounds)
        if (pair.second)
            FMOD_ErrorCheck(pair.second->release());
    m_loadedSounds.clear();

    // Release channel groups (master is owned by FMOD system)
    for (auto& kv : m_groups) {
        if (kv.second) {
            FMOD_ErrorCheck(kv.second->release());
        }
    }
    m_groups.clear();
    m_masterGroup = nullptr;

    spdlog::info("Audio: Releasing system");
    if (m_system) {

        FMOD_ErrorCheck(m_system->close());
        FMOD_ErrorCheck(m_system->release());
        m_system = nullptr;
    }

    m_initialized = false;
}

void AudioSystem::SetListenerPosition(const glm::vec3& position, const glm::vec3& velocity) noexcept {
    m_listenerPosition = position;
    if (velocity.x != 0.0f || velocity.y != 0.0f || velocity.z != 0.0f)
        m_listenerVelocity = velocity;
    else
        m_listenerMoved = true;
}

void AudioSystem::SetListenerOrientation(const glm::vec3& forward, const glm::vec3& up) noexcept {
    m_listenerForward = forward;
    m_listenerUp = up;
}

void AudioSystem::AdjustChannelVolume(AudioGroup channel, float percentDelta) {
    FMOD::ChannelGroup* group = nullptr;
    if (channel == AudioGroup::MASTER) {
        group = m_masterGroup;
    } else {
        auto it = m_groups.find(channel);
        if (it != m_groups.end())
            group = it->second;
    }
    if (!group)
        return;
    float current = 0.0f;
    FMOD_ErrorCheck(group->getVolume(&current));
    float newVol = current + percentDelta / 100.0f;
    newVol = std::clamp(newVol, 0.0f, 2.0f);
    FMOD_ErrorCheck(group->setVolume(newVol));
}

int AudioSystem::LoadSound(const std::string& dir, bool is3D, bool isStream, bool isLooping) {
	auto it = m_pathToHandle.find(dir);
	if (it != m_pathToHandle.end()) {
		m_refCounts[it->second]++;
        spdlog::warn("Audio: Duplicate load detected for '{}', reusing handle {}", dir, it->second);
        return it->second;
    }
    
    if (!m_initialized || !m_system) {
        spdlog::warn("Audio: System not initialized.");
        return -1;
	}

	spdlog::info("Audio: Loading audio: {}", dir);

	// Normalize and resolve path against engine working directory so imports work per-project
	std::string normalizedPath = dir;
	std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
	if (normalizedPath.rfind("audio/", 0) == 0) {
		normalizedPath = "assets/" + normalizedPath;
	}
	std::filesystem::path resolvedPath = std::filesystem::path(Engine::getWorkingDir()) / normalizedPath;
	resolvedPath = resolvedPath.lexically_normal();

	FMOD::Sound* sound = nullptr;

	// Sets audio characteristics
	FMOD_MODE mode = FMOD_DEFAULT;
	mode |= is3D ? FMOD_3D : FMOD_2D;
	mode |= isStream ? FMOD_CREATESTREAM : FMOD_CREATESAMPLE;
	mode |= isLooping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;

	m_result = m_system->createSound(resolvedPath.string().c_str(), mode, 0, &sound);
	if (sound == nullptr && m_result != FMOD_OK) {
		spdlog::warn("Audio: Failed to load sound at {}, {}", resolvedPath.string(), FMOD_ErrorString(m_result));
		return -1;
	}

    m_result = sound->set3DMinMaxDistance(MINDISTANCE * DISTANCEFACTOR, MAXDISTANCE * DISTANCEFACTOR);
    if (is3D && m_result != FMOD_OK)
        spdlog::warn("Audio: Failed to set 3D distance for sound {}", FMOD_ErrorString(m_result));

    // Increment handle for next audio loading
    const int handle = m_nextSoundHandle++;
    m_loadedSounds[handle] = sound;
    m_pathToHandle[dir] = handle;
    m_refCounts[handle] = 1;

    return handle;
}

void AudioSystem::UnloadSound(int soundHandle) {
    // Safety check: Don't attempt to unload during shutdown or if not initialized
    if (!m_initialized || !m_system) {
        return;
    }

    // Validate soundHandle
    if (soundHandle < 0) {
        return;
    }

    auto it = m_refCounts.find(soundHandle);
    if (it != m_refCounts.end() && --it->second <= 0) {
        if (auto s = m_loadedSounds[soundHandle]) s->release();
        m_loadedSounds.erase(soundHandle);
        for (auto pit = m_pathToHandle.begin(); pit != m_pathToHandle.end(); ++pit)
            if (pit->second == soundHandle) { m_pathToHandle.erase(pit); break; }
        m_refCounts.erase(soundHandle);
    }
}

FMOD::System* AudioSystem::GetSystem() const { return m_system; }

FMOD::Sound* AudioSystem::GetSound(int soundHandle) const {
    auto it = m_loadedSounds.find(soundHandle);
    if (it != m_loadedSounds.end())
        return it->second;
    return nullptr;
}

void AudioSystem::RegisterComponent(AudioComponent* component) {
    if (component) {
        auto it = std::find(m_components.begin(), m_components.end(), component);
        if (it == m_components.end())
            m_components.push_back(component);
    }
}

void AudioSystem::UnregisterComponent(AudioComponent* component) {
    if (component) {
        // Clean up filter DSP first (need channel to remove DSP)
        FMOD::Channel* channel = nullptr;
        auto channelIt = m_componentChannels.find(component);
        if (channelIt != m_componentChannels.end()) {
            channel = channelIt->second;
        }
        this->RemoveFilterDsp(component, channel);

        // Clean up channel if active
        if (channelIt != m_componentChannels.end()) {
            if (channelIt->second) {
                channelIt->second->stop();
            }
            m_componentChannels.erase(channelIt);
        }

        // Remove from component list
        auto it = std::find(m_components.begin(), m_components.end(), component);
        if (it != m_components.end())
            m_components.erase(it);
    }
}

bool AudioSystem::IsInitialized() const { return m_initialized; }

FMOD::Channel* AudioSystem::GetChannel(AudioComponent* component) const {
    auto it = m_componentChannels.find(component);
    return (it != m_componentChannels.end()) ? it->second : nullptr;
}

bool AudioSystem::ApplyFilterToChannel(AudioComponent* comp, FMOD::Channel* channel, const AudioFilterParams& params) {
    if (params.type == AudioFilterType::None) return true;
    if (!m_system || !channel) return false;
    FMOD_DSP_TYPE dspType = ToFMODDSPType(params.type);
    if (dspType == FMOD_DSP_TYPE_UNKNOWN) return true;
    FMOD::DSP* dsp = nullptr;
    if (m_system->createDSPByType(dspType, &dsp) != FMOD_OK || !dsp) return false;
    switch (params.type) {
        case AudioFilterType::Lowpass: {
            float cutoff = std::clamp(params.cutoffHz, 10.0f, 22050.0f);
            FMOD_ErrorCheck(dsp->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, cutoff));
            FMOD_ErrorCheck(dsp->setParameterFloat(FMOD_DSP_LOWPASS_RESONANCE, std::clamp(params.resonance, 0.5f, 10.0f)));
            break;
        }
        case AudioFilterType::Highpass: {
            float cutoff = std::clamp(params.cutoffHz, 10.0f, 22050.0f);
            FMOD_ErrorCheck(dsp->setParameterFloat(FMOD_DSP_HIGHPASS_CUTOFF, cutoff));
            FMOD_ErrorCheck(dsp->setParameterFloat(FMOD_DSP_HIGHPASS_RESONANCE, std::clamp(params.resonance, 0.5f, 10.0f)));
            break;
        }
        case AudioFilterType::Echo: {
            float delayMs = std::clamp(params.echoDelayMs, 1.0f, 5000.0f);
            float feedback = std::clamp(params.echoFeedback, 0.0f, 1.0f);
            FMOD_ErrorCheck(dsp->setParameterFloat(FMOD_DSP_ECHO_DELAY, delayMs));
            FMOD_ErrorCheck(dsp->setParameterFloat(FMOD_DSP_ECHO_FEEDBACK, feedback));
            FMOD_ErrorCheck(dsp->setParameterFloat(FMOD_DSP_ECHO_DRYLEVEL, 1.0f));
            FMOD_ErrorCheck(dsp->setParameterFloat(FMOD_DSP_ECHO_WETLEVEL, 0.5f));
            break;
        }
        default:
            break;
    }
    if (channel->addDSP(0, dsp) != FMOD_OK) {
        dsp->release();
        return false;
    }
    m_componentFilterDsp[comp] = dsp;
    return true;
}

void AudioSystem::RemoveFilterDsp(AudioComponent* comp, FMOD::Channel* channel) {
    auto it = m_componentFilterDsp.find(comp);
    if (it == m_componentFilterDsp.end()) return;
    FMOD::DSP* dsp = it->second;
    m_componentFilterDsp.erase(it);
    if (channel) FMOD_ErrorCheck(channel->removeDSP(dsp));
    FMOD_ErrorCheck(dsp->release());
}

// ============================================================================
// AudioComponent Implementation
// ============================================================================

AudioComponent::~AudioComponent() {
    // Safety check: Only cleanup if AudioSystem is still initialized
    AudioSystem& audioSys = AudioSystem::GetInstance();
    if (audioSys.IsInitialized()) {
        Stop();
        audioSys.UnregisterComponent(this);
    }
}

void AudioComponent::RefreshSoundInfo() {
    //spdlog::info("RefreshSoundInfo called: soundHandle={}, isInitialized={}", soundHandle, isInitialized);

    if (soundHandle < 0 || isInitialized) {
        //spdlog::info("RefreshSoundInfo: Early return (soundHandle={}, isInitialized={})", soundHandle, isInitialized);
        return;
    }

    FMOD::Sound* sound = AudioSystem::GetInstance().GetSound(soundHandle);
    if (!sound) {
        spdlog::warn("AudioComponent: Invalid sound handle {} - sound pointer is null", soundHandle);
        return;
    }

    // Get duration info
    unsigned int lengthMs = 0;
    if (sound->getLength(&lengthMs, FMOD_TIMEUNIT_MS) == FMOD_OK) {
        duration = lengthMs / 1000.0f;
    }

    isInitialized = true;
    //spdlog::info("RefreshSoundInfo: Successfully initialized (soundHandle={}, duration={:.2f}s)", soundHandle, duration);
    AudioSystem::GetInstance().RegisterComponent(this);

    if (playOnAwake) {
        //spdlog::info("RefreshSoundInfo: Auto-playing sound (playOnAwake=true)");
        Play();
    }
}

void AudioComponent::UpdatePosition(const glm::vec3& newPosition) {
    position = newPosition;
    FMOD::Channel* channel = AudioSystem::GetInstance().GetChannel(this);
    if (channel) {
        if (!is3D) return;
        const FMOD_VECTOR pos = ToFMOD(newPosition);
        const FMOD_VECTOR vel = ToFMOD(velocity);
        FMOD_ErrorCheck(channel->set3DAttributes(&pos, &vel));
    }
}

void AudioComponent::UpdateVelocity(const glm::vec3& newVelocity) {
    velocity = newVelocity;
    FMOD::Channel* channel = AudioSystem::GetInstance().GetChannel(this);
    if (channel) {
        if (!is3D) return;
        const FMOD_VECTOR pos = ToFMOD(position);
        const FMOD_VECTOR vel = ToFMOD(newVelocity);
        FMOD_ErrorCheck(channel->set3DAttributes(&pos, &vel));
    }
}

bool AudioComponent::Play() {
    if (!isInitialized) {
        if (!EnsureSoundLoaded(*this)) {
            //spdlog::warn("AudioComponent: Cannot play, component not initialized (soundHandle={})", soundHandle);
            return false;
        }
    }

    AudioSystem& audioSys = AudioSystem::GetInstance();
    FMOD::System* system = audioSys.GetSystem();
    FMOD::Sound* sound = audioSys.GetSound(soundHandle);

    if (!system || !sound) {
        //spdlog::warn("AudioComponent: Cannot play, system={}, sound={}",
        //             system != nullptr, sound != nullptr);
        return false;
    }

    // Stop existing playback if any
    FMOD::Channel* existingChannel = audioSys.GetChannel(this);
    if (existingChannel) {
        FMOD_ErrorCheck(existingChannel->stop());
    }

    // Start new playback
    FMOD::Channel* newChannel = nullptr;
    // Route to the requested channel group (default MASTER)
    FMOD::ChannelGroup* targetGroup = audioSys.m_masterGroup;
    if (group != AudioGroup::MASTER) {
        auto it = audioSys.m_groups.find(group);
        if (it != audioSys.m_groups.end()) {
            targetGroup = it->second;
        }
    }
    FMOD_RESULT result = system->playSound(sound, targetGroup, true, &newChannel);

    if (result != FMOD_OK) {
        spdlog::error("AudioComponent: Failed to create channel: {}", FMOD_ErrorString(result));
        return false;
    }

    if (newChannel) {
        // Apply per-component playback settings onto the CHANNEL (not the shared Sound).
        // Note: Streaming is decided at load time (createStream vs createSample). Toggling
        // AudioComponent::isStreaming at runtime won't change the already-loaded Sound.
        {
            FMOD_MODE soundMode{};
            if (sound->getMode(&soundMode) == FMOD_OK) {
                const bool loadedStreaming = (soundMode & FMOD_CREATESTREAM) != 0;
                if (loadedStreaming != isStreaming) {
                    spdlog::warn("AudioComponent: isStreaming={} but loaded sound streaming={} (streaming is set at load time; reimport/reload asset to change)",
                                 isStreaming, loadedStreaming);
                }
            }
        }

        // 2D/3D + looping are best treated as per-channel settings.
        {
            FMOD_MODE mode = FMOD_DEFAULT;
            mode |= is3D ? FMOD_3D : FMOD_2D;
            mode |= isLooping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
            FMOD_ErrorCheck(newChannel->setMode(mode));
        }

        if (is3D) {
            const FMOD_VECTOR pos = ToFMOD(position);
            const FMOD_VECTOR vel = ToFMOD(velocity);
            FMOD_ErrorCheck(newChannel->set3DAttributes(&pos, &vel));
            FMOD_ErrorCheck(newChannel->set3DMinMaxDistance(minDistance * DISTANCEFACTOR, maxDistance * DISTANCEFACTOR));
        }
        FMOD_ErrorCheck(newChannel->setVolume(volume));
        FMOD_ErrorCheck(newChannel->setLoopCount(isLooping ? -1 : 0));

        if (filterParams.type != AudioFilterType::None) {
            audioSys.ApplyFilterToChannel(this, newChannel, filterParams);
        }

        FMOD_ErrorCheck(newChannel->setPaused(false));

        // Store channel in AudioSystem
        audioSys.m_componentChannels[this] = newChannel;

        isPlaying = true;
        isPaused = false;
        playbackPosition = 0.0f;

        //const glm::vec3& listenerPos = audioSys.m_listenerPosition;
        //float distance = glm::length(position - listenerPos);

        //spdlog::info("AudioComponent: Playing sound (handle={}, volume={:.2f}, 3D={}, pos=[{:.1f},{:.1f},{:.1f}], listenerDist={:.1f}m)",
        //             soundHandle, volume, is3D, position.x, position.y, position.z, distance);

        return true;
    }

    spdlog::error("AudioComponent: Failed to create channel (newChannel is null)");
    return false;
}

bool AudioComponent::Pause() {
    FMOD::Channel* channel = AudioSystem::GetInstance().GetChannel(this);
    if (!channel)
        return false;

    if (channel->setPaused(true) == FMOD_OK) {
        isPaused = true;
        return true;
    }

    return false;
}

bool AudioComponent::Resume() {
    FMOD::Channel* channel = AudioSystem::GetInstance().GetChannel(this);
    if (!channel)
        return false;

    if (channel->setPaused(false) == FMOD_OK) {
        isPaused = false;
        isPlaying = true;
        return true;
    }

    return false;
}

bool AudioComponent::Stop() {
    AudioSystem& audioSys = AudioSystem::GetInstance();
    FMOD::Channel* channel = audioSys.GetChannel(this);
    if (!channel)
        return false;

    audioSys.RemoveFilterDsp(this, channel);

    if (channel->stop() == FMOD_OK) {
        audioSys.m_componentChannels.erase(this);
        isPlaying = false;
        isPaused = false;
        playbackPosition = 0.0f;
        return true;
    }

    return false;
}

void AudioComponent::SetLoop(bool loop) {
    isLooping = loop;

    // IMPORTANT: Do not mutate the shared FMOD::Sound loop mode here.
    // Multiple AudioComponents can reference the same Sound handle via the ResourceRegistry.
    // Looping should be treated as a per-channel playback setting.
    AudioSystem& audioSys = AudioSystem::GetInstance();
    FMOD::Channel* channel = audioSys.GetChannel(this);
    if (channel) {
        FMOD_MODE mode = FMOD_DEFAULT;
        mode |= is3D ? FMOD_3D : FMOD_2D;
        mode |= loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
        FMOD_ErrorCheck(channel->setMode(mode));
        FMOD_ErrorCheck(channel->setLoopCount(loop ? -1 : 0));
    }
}

void AudioComponent::SetVolume(float vol) {
    volume = vol;
    FMOD::Channel* channel = AudioSystem::GetInstance().GetChannel(this);
    if (channel) {
        FMOD_ErrorCheck(channel->setVolume(vol));
    }
}

void AudioComponent::SetDistanceRange(float minDist, float maxDist) {
    minDistance = minDist;
    maxDistance = maxDist;
    FMOD::Channel* channel = AudioSystem::GetInstance().GetChannel(this);
    if (channel) {
        FMOD_ErrorCheck(channel->set3DMinMaxDistance(minDist * DISTANCEFACTOR, maxDist * DISTANCEFACTOR));
    }
}

bool AudioComponent::IsPlaying() const {
    FMOD::Channel* channel = AudioSystem::GetInstance().GetChannel(const_cast<AudioComponent*>(this));
    if (!channel)
        return false;

    bool paused = false;
    bool playing = false;
    channel->getPaused(&paused);
    channel->isPlaying(&playing);

    return playing && !paused;
}

void AudioComponent::InternalUpdate() {
    AudioSystem& audioSys = AudioSystem::GetInstance();
    FMOD::Channel* channel = audioSys.GetChannel(this);
    if (!channel)
        return;

    // Update 3D attributes
    const FMOD_VECTOR pos = ToFMOD(position);
    const FMOD_VECTOR vel = ToFMOD(velocity);
    FMOD_ErrorCheck(channel->set3DAttributes(&pos, &vel));

    // Sync filter parameters if we have an active filter DSP
    {
        auto it = audioSys.m_componentFilterDsp.find(this);
        if (it != audioSys.m_componentFilterDsp.end() && it->second && filterParams.type != AudioFilterType::None) {
            SyncFilterParams(it->second, filterParams.type, filterParams);
        }
    }

    // Check playing state
    bool playing = false;
    FMOD_ErrorCheck(channel->isPlaying(&playing));
    isPlaying = playing;

    // Update playback position
    if (playing) {
        unsigned int positionMs = 0;
        if (channel->getPosition(&positionMs, FMOD_TIMEUNIT_MS) == FMOD_OK) {
            playbackPosition = positionMs / 1000.0f;
        }
    }

    // Clean up finished channel and its filter DSP
    if (!playing) {
        audioSys.RemoveFilterDsp(this, channel);
        audioSys.m_componentChannels.erase(this);
        isPlaying = false;
        isPaused = false;
        playbackPosition = 0.0f;
    }
}

// ============================================================================
// Resource Registry Integration
// ============================================================================

// Register audio resource type with the ResourceRegistry
// This allows audio assets (.audio files) to be loaded into the engine
REGISTER_RESOURCE_TYPE_ALIASE(int, audio,
    [](const char* data) -> int {
        // Deserialize AudioResourceData from binary
        AudioResourceData audioData = rp::serialization::serializer<"bin">::deserialize<AudioResourceData>(
            reinterpret_cast<const std::byte*>(data)
        );

        // Construct proper file path (prepend "assets/" if not absolute)
        std::string fullPath = audioData.sourcePath;

        // Normalize path separators to forward slashes
        std::replace(fullPath.begin(), fullPath.end(), '\\', '/');

        if (!std::filesystem::path(fullPath).is_absolute()) {
            // If path starts with "audio/", prepend "assets/"
            if (fullPath.find("audio/") == 0 || fullPath.find("audio\\") == 0) {
                fullPath = "assets/" + fullPath;
            }
        }

        // Load the actual FMOD sound via AudioSystem
        AudioSystem& audioSys = AudioSystem::GetInstance();
        int handle = audioSys.LoadSound(fullPath, audioData.is3D,
                                        audioData.isStreaming, audioData.isLooping);

        if (handle >= 0) {
            spdlog::info("Loaded audio resource: {} (handle: {}, 3D: {}, streaming: {}, loop: {})",
                fullPath, handle, audioData.is3D, audioData.isStreaming, audioData.isLooping);
        } else {
            spdlog::error("Failed to load audio resource: {} (original: {})", fullPath, audioData.sourcePath);
        }

        return handle;
    },
    [](int& handle) {
        // Cleanup: unload the FMOD sound
        if (handle >= 0) {
            AudioSystem& audioSys = AudioSystem::GetInstance();
            audioSys.UnloadSound(handle);
        }
    })
