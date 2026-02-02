#pragma once
#include <Audio.h>
using namespace DirectX;

class AudioManager
{
public:
  AudioManager();
  ~AudioManager();

  void SetVolume(float volume);
  void PlaySoundEffect(const std::wstring& soundEffectPath);
  void PlayMusic(const std::wstring& musicTrackPath, bool loop = false);
  void PauseMusic(const std::wstring& musicTrackPath);
  void ResumeMusic(const std::wstring& musicTrackPath);
  void StopMusic(const std::wstring& musicTrackPath);
  void Update();

private:
  SoundEffect& FindOrLoadSoundEffect(const std::wstring& soundEffectPath);
  SoundEffectInstance& FindOrLoadInstance(const std::wstring& musicTrackPath);
  void LoadSoundEffect(const std::wstring& soundEffectPath);
  void LoadInstance(const std::wstring& musicTrackPath);

  std::unique_ptr<AudioEngine> audioEngine;

  std::unordered_map<std::wstring, std::unique_ptr<SoundEffect>> soundEffects;
  std::unordered_map<std::wstring, std::unique_ptr<SoundEffectInstance>> musicTracks;
};

