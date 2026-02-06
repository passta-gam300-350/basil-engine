#include "pch.h"
#include "AudioManager.h"
#include <filesystem>

inline bool file_exists(const std::wstring& file)
{
  return (_waccess(file.c_str(), 0) == 0);
}

AudioManager::AudioManager() :
  audioEngine(std::make_unique<AudioEngine>(AudioEngine_Default)),
  soundEffects(),
  musicTracks()
{
  // This is only needed in Windows desktop apps
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (FAILED(hr))
	  throw std::runtime_error("Couldn't initialize the Audio Manager.");

  SetVolume(0.5f);
}

AudioManager::~AudioManager()
{
  for (auto instance = musicTracks.begin(); instance != musicTracks.end(); ++instance)
  {
    instance->second.release();
  }

  for (auto sfx = soundEffects.begin(); sfx != soundEffects.end(); ++sfx)
  {
    sfx->second.release();
  }
  
  audioEngine.release();
}

void AudioManager::SetVolume(float volume)
{
  volume = std::clamp(volume, 0.f, 1.f);
  audioEngine->SetMasterVolume(volume);
}

void AudioManager::PlaySoundEffect(const std::wstring& soundEffectPath)
{
  SoundEffect& sfx = FindOrLoadSoundEffect(soundEffectPath);
  sfx.Play();
}

void AudioManager::PlayMusic(const std::wstring& musicTrackPath, bool loop)
{
  SoundEffectInstance& music = FindOrLoadInstance(musicTrackPath);
  music.Play(loop);
}

void AudioManager::PauseMusic(const std::wstring& musicTrackPath)
{
  SoundEffectInstance& music = FindOrLoadInstance(musicTrackPath);
  music.Pause();
}

void AudioManager::ResumeMusic(const std::wstring& musicTrackPath)
{
  SoundEffectInstance& music = FindOrLoadInstance(musicTrackPath);
  music.Resume();
}

void AudioManager::StopMusic(const std::wstring& musicTrackPath)
{
  SoundEffectInstance& music = FindOrLoadInstance(musicTrackPath);
  music.Stop();
}

void AudioManager::Update()
{
  audioEngine->Update();
}

SoundEffect& AudioManager::FindOrLoadSoundEffect(const std::wstring& soundEffectPath)
{
  if (soundEffects.count(soundEffectPath) < 1)
  {
    LoadSoundEffect(soundEffectPath);
  }

  auto sfxIt = soundEffects.find(soundEffectPath);
  assert(sfxIt != soundEffects.end());
  return *sfxIt->second;
}

SoundEffectInstance& AudioManager::FindOrLoadInstance(const std::wstring& musicTrackPath)
{
  if (musicTracks.count(musicTrackPath) < 1)
  {
    LoadInstance(musicTrackPath);
  }

  auto sfxIt = musicTracks.find(musicTrackPath);
  assert(sfxIt != musicTracks.end());
  return *sfxIt->second;
}

void AudioManager::LoadSoundEffect(const std::wstring& soundEffectPath)
{
  if (!file_exists(soundEffectPath))
  {
    throw std::runtime_error("SFX file does not exist!");
    return; // This line added for students. If you're here, that file doesn't exist
  }

  auto result_pair = soundEffects.emplace(soundEffectPath, std::make_unique<SoundEffect>(audioEngine.get(), soundEffectPath.c_str()));
  if (!result_pair.second) // Wasn't loaded correctly
  {
    throw std::runtime_error("Couldn't load sound effect.");
  }
}

void AudioManager::LoadInstance(const std::wstring& musicTrackPath)
{
  SoundEffect& sfx = FindOrLoadSoundEffect(musicTrackPath);
  std::unique_ptr<SoundEffectInstance> instance = sfx.CreateInstance(SoundEffectInstance_ReverbUseFilters);

  auto result_pair = musicTracks.emplace(musicTrackPath, std::move(instance));
  if (!result_pair.second) // Wasn't loaded correctly
  {
    throw std::runtime_error("Couldn't load music track.");
  }
}
