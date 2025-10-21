//#include "System/Audio.hpp"
//#include <vector>
//
//namespace Audio
//{
//	FMOD_RESULT result;
//	FMOD::System* p_system;
//	FMOD::ChannelGroup* master_group;
//	std::vector<FMOD::ChannelGroup*> v_group;		// Need to pull data of created channel groups
//	std::unordered_map<std::string, FMOD::Sound*> m_sound;
//	std::vector<FMOD::Channel*> v_channel;
//	FMOD_MODE mode = FMOD_DEFAULT;
//	bool paused{ false };
//
//#pragma region TEMP VARIABLES & FUNCTIONS
//	FMOD_VECTOR listener_pos{ 0.0f, 0.0f, 0.0f };
//	FMOD_VECTOR listener_vel{ 0.0f, 0.0f, 0.0f };
//	FMOD_VECTOR sound_pos{ 0.0f, 0.0f, 0.0f };
//	FMOD_VECTOR sound_vel{ 0.0f, 0.0f, 0.0f };
//	FMOD_VECTOR right{ 1.0f, 0.0f, 0.0f };
//	FMOD_VECTOR up{ 0.0f, 1.0f, 0.0f };
//	FMOD_VECTOR forward{ 0.0f, 0.0f, 1.0f };
//	//FMOD::Sound* p_sound{ 0 };
//	//FMOD::Channel* p_channel{ 0 };
//	std::vector<FMOD::DSP*> dsp = { 0, 0, 0, 0 };
//	std::vector<bool> dsp_bypass = {true, true, true, true};
//#pragma endregion
//
//#pragma region Helper Functions
//	void FMOD_ErrorCheck(FMOD_RESULT _result)
//	{
//		if (_result != FMOD_OK)
//		{
//			std::cerr << "FMOD ERROR! " << FMOD_ErrorString(_result);
//			assert(false);
//			//exit(-1);
//		}
//	}
//#pragma endregion
//
//#pragma region Private Functions
//	void Play_Audio()
//	{
//		std::cout << "Playing audio\n";
//		FMOD::Channel* p_channel = nullptr;
//		result = p_system->playSound(m_sound[0], nullptr, false, &p_channel);
//	}
//
//	void Stop_Audio()
//	{
//		std::cout << "Stopping audio\n";
//		if(paused)
//		{
//			std::cerr << "WARNING: Stopping audio on a paused channel!\n";
//			return;
//		}
//	}
//
//	void Stop_All_Audio()
//	{
//		for (int i{ 0 }; i < v_group.size(); ++i)
//		{
//			result = v_group[i]->stop();
//			FMOD_ErrorCheck(result);
//		}
//	}
//
//	void Load_Audio(std::string _dir, bool _loop, bool _stream, bool _3d, bool _ambient)
//	{
//		std::cout << "Loading audio\n";
//		mode |= (_loop) ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
//		mode |= (_stream) ? FMOD_CREATESTREAM : FMOD_CREATESAMPLE;
//		mode |= (_3d) ? FMOD_3D : FMOD_2D;
//		if (_3d)
//			mode |= (_ambient) ? FMOD_3D_INVERSETAPEREDROLLOFF : FMOD_3D_INVERSEROLLOFF;
//		FMOD::Sound* p_sound = nullptr;
//		result = p_system->createSound(_dir.c_str(), mode, nullptr, &p_sound);
//		FMOD_ErrorCheck(result);
//		if (_3d)
//		{
//			result = p_sound->set3DMinMaxDistance(MINDISTANCE, MAXDISTANCE);
//			FMOD_ErrorCheck(result);
//		}
//		if (p_sound)
//			m_sound[_dir] = p_sound;
//	}
//
//	void Unload_Audio()
//	{
//		std::cout << "Unloading audio\n";
//	}
//
//	void Unload_All_Audio()
//	{
//		for (auto& sound : m_sound)
//		{
//			result = sound.second->release();
//			FMOD_ErrorCheck(result);
//		}
//		m_sound.clear();
//	}
//
//	FMOD_MODE Mode_Selector(Sound_Mode _mode)
//	{
//		switch (_mode)
//		{
//		case LOOP:		return FMOD_LOOP_NORMAL;
//		case NO_LOOP:	return FMOD_LOOP_OFF;
//		default:		return FMOD_DEFAULT;
//		}
//	}
//
//	FMOD_DSP_TYPE Filter_Selector(Filter _filter)
//	{
//		switch (_filter)
//		{
//		case LOW_PASS:		return FMOD_DSP_TYPE_LOWPASS;
//		case HIGH_PASS:		return FMOD_DSP_TYPE_HIGHPASS;
//		case ECHO:			return FMOD_DSP_TYPE_ECHO;
//		case DISTORTION:	return FMOD_DSP_TYPE_DISTORTION;
//		default:			return FMOD_DSP_TYPE_UNKNOWN;
//		}
//	}
//#pragma endregion
//
//#pragma region Public Functions
//	void Init(void* extradriverdata)
//	{
//		std::cout << "Creating audio system\n";
//		result = FMOD::System_Create(&p_system);
//		FMOD_ErrorCheck(result);
//
//		std::cout << "Initializing FMOD\n";
//		result = p_system->init(512, FMOD_INIT_NORMAL, extradriverdata);
//		FMOD_ErrorCheck(result);
//
//		std::cout << "Setting 3D parameters\n";
//		result = p_system->set3DSettings(DOPPLERSCALE, DISTANCEFACTOR, ROLLOFFSCALE); //TEMP (Set saved 3D settings)
//		FMOD_ErrorCheck(result);
//
//		// Need to pull data of created channel groups
//		std::cout << "Creating Channel Groups\n";
//		v_group.resize(1);
//		for (int i{ 0 }; i < v_group.size(); ++i)
//		{
//			result = p_system->createChannelGroup(nullptr, &v_group[i]);
//			FMOD_ErrorCheck(result);
//			// result = Set saved volume here
//			// FMOD_ErrorCheck(result);
//		}
//
//		//TEMP
//		// Set channel to channel group
//		result = v_channel[0]->setChannelGroup(v_group[0]);
//
//		std::cout << "Getting Master Channel Group\n";
//		result = p_system->getMasterChannelGroup(&master_group);
//		FMOD_ErrorCheck(result);
//		result = master_group->setVolume(0.5f); //TEMP (Set saved master volume)
//		FMOD_ErrorCheck(result);
//
//		//TEMP
//		// createSound here when serialization is done
//
//		std::cout << "Creating drumloop.wave\n";
//		Load_Audio("", true, true, false); // TEMP (Add path to audio file)
//		
//		result = p_system->playSound(m_sound[0], nullptr, false, &v_channel[0]);
//		FMOD_ErrorCheck(result);
//
//		// createDSP every possible built in DSP (custom DSP plugins when serialization is done)
//		dsp.reserve(SIZE_OF_FILTERS);
//
//		for (int i{ 0 }; i < SIZE_OF_FILTERS; ++i)
//		{
//			result = p_system->createDSPByType(Filter_Selector(static_cast<Filter>(i)), &dsp[i]);
//			FMOD_ErrorCheck(result);
//			result = v_channel[0]->addDSP(0, dsp[i]);
//			FMOD_ErrorCheck(result);
//			result = dsp[i]->setBypass(dsp_bypass[i]);
//			FMOD_ErrorCheck(result);
//		}
//		// Need to find a way to separate DSP on different channels (DSP is shared across all channels)
//
//		//ENDTEMP
//	}
//
//	void Update()
//	{
//		std::vector<FMOD::Channel*> stopped_channels;
//		for (auto it = v_channel.begin(); it != v_channel.end(); ++it)
//		{
//			bool is_playing = false;
//			result = (*it)->isPlaying(&is_playing);
//			FMOD_ErrorCheck(result);
//			if (!is_playing)
//			{
//				stopped_channels.push_back(*it);
//			}
//		}
//		for (auto& channel : stopped_channels)
//		{
//			auto it = std::find(v_channel.begin(), v_channel.end(), channel);
//			if (it != v_channel.end())
//			{
//				v_channel.erase(it);
//			}
//		}
//		//TEMP
//		//{
//		//	if (Common_BtnDown(BTN_Q))
//		//	{
//		//		listener_pos.y -= 1.0f * DISTANCEFACTOR;
//		//		if (listener_pos.y < -20.0f * DISTANCEFACTOR)
//		//		{
//		//			listener_pos.y = -20.0f * DISTANCEFACTOR;
//		//		}
//		//	}
//
//		//	if (Common_BtnDown(BTN_E))
//		//	{
//		//		listener_pos.y += 1.0f * DISTANCEFACTOR;
//		//		if (listener_pos.y > 20.0f * DISTANCEFACTOR)
//		//		{
//		//			listener_pos.y = 20.0f * DISTANCEFACTOR;
//		//		}
//		//	}
//
//		//	if (Common_BtnDown(BTN_A))
//		//	{
//		//		listener_pos.x += 1.0f * DISTANCEFACTOR;
//		//		if (listener_pos.x > 20.0f * DISTANCEFACTOR)
//		//		{
//		//			listener_pos.x = 20.0f * DISTANCEFACTOR;
//		//		}
//		//	}
//
//		//	if (Common_BtnDown(BTN_D))
//		//	{
//		//		listener_pos.x -= 1.0f * DISTANCEFACTOR;
//		//		if (listener_pos.x < -20.0f * DISTANCEFACTOR)
//		//		{
//		//			listener_pos.x = -20.0f * DISTANCEFACTOR;
//		//		}
//		//	}
//
//		//	if (Common_BtnDown(BTN_W))
//		//	{
//		//		listener_pos.z += 1.0f * DISTANCEFACTOR;
//		//		if (listener_pos.z > 20.0f * DISTANCEFACTOR)
//		//		{
//		//			listener_pos.z = 20.0f * DISTANCEFACTOR;
//		//		}
//		//	}
//
//		//	if (Common_BtnDown(BTN_S))
//		//	{
//		//		listener_pos.z -= 1.0f * DISTANCEFACTOR;
//		//		if (listener_pos.z < -20.0f * DISTANCEFACTOR)
//		//		{
//		//			listener_pos.z = -20.0f * DISTANCEFACTOR;
//		//		}
//		//	}
//
//		//	if (Common_BtnDown(BTN_Y))
//		//	{
//		//		sound_pos.y -= 1.0f * DISTANCEFACTOR;
//		//		if (sound_pos.y < -20.0f * DISTANCEFACTOR)
//		//		{
//		//			sound_pos.y = -20.0f * DISTANCEFACTOR;
//		//		}
//		//	}
//
//		//	if (Common_BtnDown(BTN_I))
//		//	{
//		//		sound_pos.y += 1.0f * DISTANCEFACTOR;
//		//		if (sound_pos.y > 20.0f * DISTANCEFACTOR)
//		//		{
//		//			sound_pos.y = 20.0f * DISTANCEFACTOR;
//		//		}
//		//	}
//
//		//	if (Common_BtnDown(BTN_H))
//		//	{
//		//		sound_pos.x += 1.0f * DISTANCEFACTOR;
//		//		if (sound_pos.x > 20.0f * DISTANCEFACTOR)
//		//		{
//		//			sound_pos.x = 20.0f * DISTANCEFACTOR;
//		//		}
//		//	}
//
//		//	if (Common_BtnDown(BTN_K))
//		//	{
//		//		sound_pos.x -= 1.0f * DISTANCEFACTOR;
//		//		if (sound_pos.x < -20.0f * DISTANCEFACTOR)
//		//		{
//		//			sound_pos.x = -20.0f * DISTANCEFACTOR;
//		//		}
//		//	}
//
//		//	if (Common_BtnDown(BTN_U))
//		//	{
//		//		sound_pos.z += 1.0f * DISTANCEFACTOR;
//		//		if (sound_pos.z > 20.0f * DISTANCEFACTOR)
//		//		{
//		//			sound_pos.z = 20.0f * DISTANCEFACTOR;
//		//		}
//		//	}
//
//		//	if (Common_BtnDown(BTN_J))
//		//	{
//		//		sound_pos.z -= 1.0f * DISTANCEFACTOR;
//		//		if (sound_pos.z < -20.0f * DISTANCEFACTOR)
//		//		{
//		//			sound_pos.z = -20.0f * DISTANCEFACTOR;
//		//		}
//		//	}
//
//		//	if (Common_BtnPress(BTN_MORE))
//		//	{
//		//		paused = !paused;
//		//		if (v_channel[0])
//		//		{
//		//			result = v_channel[0]->setPaused(paused);
//		//			FMOD_ErrorCheck(result);
//		//		}
//		//	}
//		//}
//		//ENDTEMP
//
//		result = p_system->set3DListenerAttributes(0, &listener_pos, &listener_vel, &forward, &up);
//		FMOD_ErrorCheck(result);
//
//		result = v_channel[0]->set3DAttributes(&sound_pos, &sound_vel);
//		FMOD_ErrorCheck(result);
//
//		//TEMP
//		//{
//		//	if (Common_BtnPress(BTN_ACTION1))
//		//	{
//		//		dsp_bypass[0] = !dsp_bypass[0];
//		//		result = dsp[0]->setBypass(dsp_bypass[0]);
//		//		FMOD_ErrorCheck(result);
//		//	}
//
//		//	if (Common_BtnPress(BTN_ACTION2))
//		//	{
//		//		dsp_bypass[1] = !dsp_bypass[1];
//		//		result = dsp[1]->setBypass(dsp_bypass[1]);
//		//		FMOD_ErrorCheck(result);
//		//	}
//
//		//	if (Common_BtnPress(BTN_ACTION3))
//		//	{
//		//		dsp_bypass[2] = !dsp_bypass[2];
//		//		result = dsp[2]->setBypass(dsp_bypass[2]);
//		//		FMOD_ErrorCheck(result);
//		//	}
//
//		//	if (Common_BtnPress(BTN_ACTION4))
//		//	{
//		//		dsp_bypass[3] = !dsp_bypass[3];
//		//		result = dsp[3]->setBypass(dsp_bypass[3]);
//		//		FMOD_ErrorCheck(result);
//		//	}
//		//}
//		//ENDTEMP
//
//		result = p_system->update();
//		FMOD_ErrorCheck(result);
//	}
//
//	void Exit()
//	{
//		std::cout << "Exiting\n";
//		//Release all created sounds here
//		for (auto& sound : m_sound)
//		{
//			result = sound.second->release();
//			FMOD_ErrorCheck(result);
//		}
//		FMOD_ErrorCheck(result);
//		//Release all created channel groups here
//		result = p_system->release();
//		FMOD_ErrorCheck(result);
//	}
//#pragma endregion
//}