#ifndef LIB_RESOURCE_CORE_NATIVE_AUDIO_H
#define LIB_RESOURCE_CORE_NATIVE_AUDIO_H

#include <string>
#include <cstdint>
#include "serialization/native_serializer.h"

// Audio mixing groups (shared between resource system and engine)
// NOTE: Keep values stable for serialization/editor.
enum class AudioGroup : std::uint8_t {
    MASTER = 0,
    BGM,
    SFX,
    UI,
    AMBIENT
};

// Audio asset metadata (runtime native data)
struct AudioResourceData {
    std::string sourcePath;  // Relative path to audio file
    bool is3D = true;
    bool isStreaming = false;
    bool isLooping = false;
    AudioGroup group = AudioGroup::MASTER; // Default to MASTER
    float duration = 0.0f; // in seconds
    int sampleRate = 0;
    int channels = 0;
};

// Explicit reflection metadata for AudioResourceData to ensure correct field names
template <>
struct rp::reflection::ExternalTypeMetadata<AudioResourceData> {
    using ExternalTypeBinder = rp::reflection::ExternalTypeBinderMetadata<AudioResourceData,
        &AudioResourceData::sourcePath,
        &AudioResourceData::is3D,
        &AudioResourceData::isStreaming,
        &AudioResourceData::isLooping,
        &AudioResourceData::group,
        &AudioResourceData::duration,
        &AudioResourceData::sampleRate,
        &AudioResourceData::channels
    >;
};

#endif
