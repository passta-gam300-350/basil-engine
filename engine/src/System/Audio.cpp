#include "System/Audio.hpp"
//
//FMOD::System* p_system;
//FMOD::ChannelGroup* master_group;
//std::unordered_map<std::string, FMOD::ChannelGroup*> v_group;		// Need to pull data of created channel groups
//std::unordered_map<std::string, FMOD::Sound*> m_sound;
//std::vector<FMOD::Channel*> v_channel;
//bool paused{ false };
//static std::unordered_map<FMOD::Channel*, std::vector<FMOD::DSP*>> channelToDsps;
//static std::unordered_map<FMOD::Channel*, std::pair<glm::vec3, glm::vec3>> channelTo3DPos; // channel -> (position, velocity)
//
//#pragma region TEMP VARIABLES & FUNCTIONS
//glm::vec3 listener_pos{ 0.0f, 0.0f, 0.0f };
//glm::vec3 listener_vel{ 0.0f, 0.0f, 0.0f };
//FMOD_VECTOR sound_pos{ 0.0f, 0.0f, 0.0f };
//FMOD_VECTOR sound_vel{ 0.0f, 0.0f, 0.0f };
//glm::vec3 right{ 1.0f, 0.0f, 0.0f };
//glm::vec3 up{ 0.0f, 1.0f, 0.0f };
//glm::vec3 forward{ 0.0f, 0.0f, 1.0f };
//std::vector<FMOD::DSP*> dsp = { 0, 0, 0, 0 };
//std::vector<bool> dsp_bypass = { true, true, true, true };
//#pragma endregion
//
//void AudioSystem::FMOD_ErrorCheck(FMOD_RESULT _result) {
//	if (_result != FMOD_OK)
//	{
//		spdlog::warn("FMOD ERROR! {}", FMOD_ErrorString(_result));
//		assert(false);
//		//exit(-1);
//	}
//}
//
//FMOD_VECTOR AudioSystem::Vec3_To_FMOD(const glm::vec3& v) noexcept {
//	FMOD_VECTOR fVec;
//	fVec.x = v.x;
//	fVec.y = v.y;
//	fVec.z = v.z;
//	return fVec;
//}
//
//glm::vec3 AudioSystem::FMOD_to_Vec3(const FMOD_VECTOR& fv) noexcept {
//	return glm::vec3(fv.x, fv.y, fv.z);
//}
//
//void AudioSystem::Play_Audio(const std::string& path, float volume, std::string group) {
//    auto it = m_sound.find(path);
//    if (it == m_sound.end()) {
//		spdlog::warn("Audio: Sound not loaded: {}", path);
//        return;
//    }
//    FMOD::Sound* p_sound = it->second;
//    FMOD::Channel* ch = nullptr;
//    FMOD_ErrorCheck(p_system->playSound(p_sound, nullptr, false, &ch));
//	FMOD_ErrorCheck(ch->setVolume(volume));
//	spdlog::info("Audio: Playing audio: {}", path);
//    if (ch) {
//        // Check for existing channel group; do not create here
//        auto gIt = v_group.find(group);
//        if (gIt == v_group.end() || !gIt->second) {
//            spdlog::warn("Audio: Channel group not found: {}", group);
//            FMOD_ErrorCheck(ch->stop());
//            return;
//        }
//        FMOD_ErrorCheck(ch->setChannelGroup(gIt->second));
//        v_channel.push_back(ch);
//    }
//}
//
//void AudioSystem::Stop_Audio() {
//	spdlog::info("Audio: Stopping audio");
//	if (paused)
//	{
//		spdlog::warn("Audio: Stopping audio on a paused channel!");
//		return;
//	}
//}
//
//void AudioSystem::Stop_All_Audio() {
//	spdlog::info("Audio: Stopping all audio");
//	for (const auto& group : v_group)
//		FMOD_ErrorCheck(group.second->stop());
//}
//
//void AudioSystem::Load_Audio(std::string dir, bool stream, bool ambient, bool dimension, bool linear, bool playOnAwake, bool loop, float minDistance, float maxDistance) {
//	FMOD::Sound* p_sound = nullptr;
//	FMOD_MODE mode = FMOD_DEFAULT;
//	mode |= (stream) ? FMOD_CREATESTREAM : FMOD_CREATESAMPLE;
//	mode |= (dimension) ? FMOD_3D : FMOD_2D;
//	mode |= (loop) ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
//	if (dimension) mode |= (ambient) ? FMOD_3D_INVERSETAPEREDROLLOFF : FMOD_3D_INVERSEROLLOFF;
//    mode |= (linear) ? FMOD_3D_LINEARROLLOFF : FMOD_3D_INVERSETAPEREDROLLOFF; // [TEMP] Just to test linear rolloff
//	FMOD_ErrorCheck(p_system->createSound(dir.c_str(), mode, nullptr, &p_sound));
//	if (p_sound) {
//		if (dimension)
//			FMOD_ErrorCheck(p_sound->set3DMinMaxDistance(minDistance, maxDistance));
//		m_sound[dir] = p_sound;
//	}
//	spdlog::info("Audio: Loading audio: {}", dir);
//}
//
//void AudioSystem::Unload_Audio() {
//	spdlog::info("Audio: Unloading audio");
//}
//
//void AudioSystem::Unload_All_Audio() {
//	spdlog::info("Audio: Unloading all audio");
//	for (auto& sound : m_sound)
//		FMOD_ErrorCheck(sound.second->release());
//	m_sound.clear();
//}
//
//FMOD_MODE AudioSystem::Mode_Selector(Sound_Mode _mode) noexcept {
//	switch (_mode)
//	{
//	case LOOP:		return FMOD_LOOP_NORMAL;
//	case NO_LOOP:	return FMOD_LOOP_OFF;
//	default:		return FMOD_DEFAULT;
//	}
//}
//
//FMOD_DSP_TYPE AudioSystem::Filter_Selector(Filter _filter) noexcept {
//	switch (_filter)
//	{
//	case LOW_PASS:		return FMOD_DSP_TYPE_LOWPASS;
//	case HIGH_PASS:		return FMOD_DSP_TYPE_HIGHPASS;
//	case ECHO:			return FMOD_DSP_TYPE_ECHO;
//	case DISTORTION:	return FMOD_DSP_TYPE_DISTORTION;
//	default:			return FMOD_DSP_TYPE_UNKNOWN;
//	}
//}
//
//float  AudioSystem::dbToVolume(float dB) noexcept {
//	return powf(10.0f, 0.05f * dB);
//}
//
//float  AudioSystem::VolumeTodB(float volume) noexcept {
//	return 20.0f * log10f(volume);
//}
//
//void AudioSystem::Set3DListenerAttributes(const glm::vec3& vPos, const glm::vec3& vVel, const glm::vec3& vFwd, const glm::vec3& vUp) {
//	const FMOD_VECTOR fPos = Vec3_To_FMOD(vPos);
//	const FMOD_VECTOR fVel = Vec3_To_FMOD(vVel);
//	const FMOD_VECTOR fFwd = Vec3_To_FMOD(vFwd);
//	const FMOD_VECTOR fUp = Vec3_To_FMOD(vUp);
//	FMOD_ErrorCheck(p_system->set3DListenerAttributes(0, &fPos, &fVel, &fFwd, &fUp));
//}
//
//void AudioSystem::Set3DSoundAttributes(FMOD::Channel* ch, const glm::vec3& vPos, const glm::vec3& vVel) {
//	if (!ch) return;
//	const FMOD_VECTOR pos = Vec3_To_FMOD(vPos);
//	const FMOD_VECTOR vel = Vec3_To_FMOD(vVel);
//	FMOD_ErrorCheck(ch->set3DAttributes(&pos, &vel));
//}
//
//void AudioSystem::RegisterChannel3DPosition(FMOD::Channel* ch, const glm::vec3& vPos, const glm::vec3& vVel) {
//	if (!ch) return;
//	channelTo3DPos[ch] = { vPos, vVel };
//}
//
//void AudioSystem::UpdateAllChannel3DAttributes() {
//	for (auto it = channelTo3DPos.begin(); it != channelTo3DPos.end();) {
//		FMOD::Channel* ch = it->first;
//		if (!ch) {
//			it = channelTo3DPos.erase(it);
//			continue;
//		}
//		
//		bool is_playing = false;
//		if (ch->isPlaying(&is_playing) == FMOD_OK && is_playing) {
//			Set3DSoundAttributes(ch, it->second.first, it->second.second);
//			++it;
//		} else {
//			it = channelTo3DPos.erase(it);
//		}
//	}
//}
//
//AudioSystem AudioSystem::System() noexcept {
//	return AudioSystem();
//}
//
//void AudioSystem::Init(void* extradriverdata) {
//	spdlog::info("Audio: Creating audio system");
//	FMOD_ErrorCheck(FMOD::System_Create(&p_system));
//
//	spdlog::info("Audio: Initializing FMOD");
//	FMOD_ErrorCheck(p_system->init(512, FMOD_INIT_NORMAL, extradriverdata));
//
//	spdlog::info("Audio: Setting 3D parameters");
//	FMOD_ErrorCheck(p_system->set3DSettings(DOPPLERSCALE, DISTANCEFACTOR, ROLLOFFSCALE)); //TEMP (Set saved 3D settings)
//
//	// Need to pull data of created channel groups
//	spdlog::info("Audio: Creating Channel Groups");
//
//	// [TEMP]
//
//	spdlog::info("Audio: Getting Master Channel Group");
//	FMOD_ErrorCheck(p_system->getMasterChannelGroup(&master_group));
//	FMOD_ErrorCheck(master_group->setVolume(0.5f)); // [TEMP] (Set saved master volume)
//	// Register MASTER in the channel group map for easy access
//	v_group["MASTER"] = master_group;
//
//	// createSound here when serialization is done
//
//	// [ENDTEMP]
//}
//
//
//void AudioSystem::Update(ecs::world&) {
//	std::vector<FMOD::Channel*> stopped_channels;
//	for (auto it = v_channel.begin(); it != v_channel.end(); ++it) {
//		FMOD::Channel* ch = *it;
//		if (!ch)
//			continue;
//		bool is_playing = false;
//		FMOD_ErrorCheck(ch->isPlaying(&is_playing));
//		if (!is_playing)
//			stopped_channels.push_back(ch);
//	}
//
//	for (auto& channel : stopped_channels) {
//		auto it = std::find(v_channel.begin(), v_channel.end(), channel);
//		if (it != v_channel.end()){
//			auto d = channelToDsps.find(*it);
//			if (d != channelToDsps.end()) {
//				for (auto* node : d->second) {
//					if (!node) continue;
//					node->release();
//				}
//				channelToDsps.erase(d);
//			}
//			channelTo3DPos.erase(*it);
//			v_channel.erase(it);
//		}
//	}
//
//	Set3DListenerAttributes(listener_pos, listener_vel, forward, up);
//	UpdateAllChannel3DAttributes();
//
//	// [TEMP] Update temp sound position (remove this once ECS integration is complete)
//	if (!v_channel.empty() && v_channel[0]) {
//		static auto startTime = std::chrono::steady_clock::now();
//		const auto currentTime = std::chrono::steady_clock::now();
//		auto elapsed = std::chrono::duration<float>(currentTime - startTime).count();
//		
//		constexpr float radius = 10.0f * DISTANCEFACTOR;
//		const float speed = 0.1f;
//		const float angle = elapsed * speed * 2.0f * 3.14159265f;
//		
//		sound_pos.x = radius * std::cos(angle);
//		sound_pos.y = 0.0f;
//		sound_pos.z = radius * std::sin(angle);
//		
//		sound_vel.x = -radius * speed * 2.0f * 3.14159265f * std::sin(angle);
//		sound_vel.y = 0.0f;
//		sound_vel.z = radius * speed * 2.0f * 3.14159265f * std::cos(angle);
//		
//		const glm::vec3 vPos = FMOD_to_Vec3(sound_pos);
//		const glm::vec3 vVel = FMOD_to_Vec3(sound_vel);
//		Set3DSoundAttributes(v_channel[0], vPos, vVel);
//		RegisterChannel3DPosition(v_channel[0], vPos, vVel);
//	}
//
//	FMOD_ErrorCheck(p_system->update());
//}
//
//
//void AudioSystem::Exit() {
//	spdlog::info("Audio: Exiting");
//	Unload_All_Audio();
//	//Release all created channel groups here
//	//
//	FMOD_ErrorCheck(p_system->release());
//}

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

    m_listenerPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    m_listenerVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
    m_listenerLastPosition = m_listenerPosition;
    m_listenerMoved = false;

    m_initialized = true;
    return true;
}

void AudioSystem::Update(ecs::world& world) {
    if (!m_initialized || !m_system)
        return;

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

    for (auto* component : m_components)
        if (component)
            component->InternalUpdate();

    FMOD_ErrorCheck(m_system->update());
}

void AudioSystem::Exit() {
    spdlog::info("Audio: Exiting");
    if (!m_initialized)  return;

    spdlog::info("Audio: Unregistering audio components");
    m_components.clear();

    spdlog::info("Audio: Releasing sounds");
    for (auto& pair : m_loadedSounds)
        if (pair.second)
            FMOD_ErrorCheck(pair.second->release());
    m_loadedSounds.clear();

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

    FMOD::Sound* sound = nullptr;

    FMOD_MODE mode = FMOD_DEFAULT;
	mode |= is3D ? FMOD_3D : FMOD_2D;
	mode |= isStream ? FMOD_CREATESTREAM : FMOD_CREATESAMPLE;
	mode |= isLooping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;

    m_result = m_system->createSound(dir.c_str(), mode, 0, &sound);
    if (sound == nullptr && m_result != FMOD_OK) {
        spdlog::warn("Audio: Failed to load sound at {}, {}", dir, FMOD_ErrorString(m_result));
        return -1;
    }

    m_result = sound->set3DMinMaxDistance(MINDISTANCE * DISTANCEFACTOR, MAXDISTANCE * DISTANCEFACTOR);
    if (is3D && m_result != FMOD_OK)
        spdlog::warn("Audio: Failed to set 3D distance for sound {}", FMOD_ErrorString(m_result));

    const int handle = m_nextSoundHandle++;
    m_loadedSounds[handle] = sound;
    m_pathToHandle[dir] = handle;
    m_refCounts[handle] = 1;

    return handle;
}

void AudioSystem::UnloadSound(int soundHandle) {
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
        auto it = std::find(m_components.begin(), m_components.end(), component);
        if (it != m_components.end())
            m_components.erase(it);
    }
}

bool AudioSystem::IsInitialized() const { return m_initialized; }

// ============================================================================
// AudioComponent Implementation
// ============================================================================

AudioComponent::~AudioComponent() {
    Stop();
    AudioSystem::GetInstance().UnregisterComponent(this);
}

bool AudioComponent::Init(int handle) {
    if (isInitialized) return true;

    soundHandle = handle;

    sound = AudioSystem::GetInstance().GetSound(handle);
    if (!sound) {
        spdlog::warn("AudioComponent: Invalid sound handle {}", handle);
        return false;
    }

    isInitialized = true;
    AudioSystem::GetInstance().RegisterComponent(this);

    if (playOnAwake)
        Play();

    return true;
}

bool AudioComponent::Init(const std::string& filePath, bool is3D, bool isStream, bool isLooping) {
    int handle = AudioSystem::GetInstance().LoadSound(AUDIO_PATH + filePath, is3D, isStream, isLooping);
    spdlog::info("AudioComponent: Initializing sound from {}", AUDIO_PATH + filePath);
    if (handle == -1){
        spdlog::warn("AudioComponent: Invalid sound handle {}", handle);
        return false;
    }

    return Init(handle);
}

void AudioComponent::UpdatePosition(const glm::vec3& newPosition) {
    if (channel) {
        const FMOD_VECTOR pos = ToFMOD(newPosition);
        const FMOD_VECTOR vel = ToFMOD(velocity);
        FMOD_ErrorCheck(channel->set3DAttributes(&pos, &vel));
    }
}

void AudioComponent::UpdateVelocity(const glm::vec3& newVelocity) {
    if (channel) {
        const FMOD_VECTOR pos = ToFMOD(position);
        const FMOD_VECTOR vel = ToFMOD(newVelocity);
        FMOD_ErrorCheck(channel->set3DAttributes(&pos, &vel));
    }
}

bool AudioComponent::Play() {
    if (!isInitialized || !sound) {
        spdlog::warn("AudioComponent: Cannot play, component not initialized");
        return false;
    }

    FMOD::System* system = AudioSystem::GetInstance().GetSystem();
    if (!system)
        return false;

    if (channel) {
        FMOD_ErrorCheck(channel->stop());
        channel = nullptr;
    }

    FMOD_ErrorCheck(system->playSound(sound, 0, true, &channel));
    const FMOD_VECTOR pos = ToFMOD(position);
    const FMOD_VECTOR vel = ToFMOD(velocity);
    FMOD_ErrorCheck(channel->set3DAttributes(&pos, &vel));
    FMOD_ErrorCheck(channel->set3DMinMaxDistance(minDistance * DISTANCEFACTOR, maxDistance * DISTANCEFACTOR));
    FMOD_ErrorCheck(channel->setVolume(volume));
    FMOD_ErrorCheck(channel->setPaused(false));

    isPlaying = true;
    isPaused = false;

    return true;
}

bool AudioComponent::Pause() {
    if (!channel)
        return false;

    if (channel->setPaused(true) == FMOD_OK) {
        isPaused = true;
        return true;
    }

    return false;
}

bool AudioComponent::Resume() {
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
    if (!channel)
        return false;

    if (channel->stop() == FMOD_OK) {
        channel = nullptr;
        isPlaying = false;
        isPaused = false;
        return true;
    }

    return false;
}

void AudioComponent::SetVolume(float vol) {
    if (channel) {
        volume = vol;
        FMOD_ErrorCheck(channel->setVolume(dbToVolume(vol)));
    }
}

void AudioComponent::SetDistanceRange(float minDist, float maxDist) {
    if (channel) {
        minDistance = minDist;
		maxDistance = maxDist;
        FMOD_ErrorCheck(channel->set3DMinMaxDistance(minDist * DISTANCEFACTOR, maxDist * DISTANCEFACTOR));
    }
}

bool AudioComponent::IsPlaying() const {
    if (!channel)
        return false;

    bool paused = false;
    bool playing = false;
    channel->getPaused(&paused);
    channel->isPlaying(&playing);

    return playing && !paused;
}

void AudioComponent::InternalUpdate() {
    if (!channel)
        return;

    const FMOD_VECTOR pos = ToFMOD(position);
    const FMOD_VECTOR vel = ToFMOD(velocity);
    FMOD_ErrorCheck(channel->set3DAttributes(&pos, &vel));

    bool playing = false;
    FMOD_ErrorCheck(channel->isPlaying(&playing));
    isPlaying = playing;

    if (!playing) {
        channel = nullptr;
        isPlaying = false;
        isPaused = false;
    }
}
