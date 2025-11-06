#ifndef LIB_RESOURCE_CORE_NATIVE_AUDIO_H
#define LIB_RESOURCE_CORE_NATIVE_AUDIO_H

#include <string>
#include "serialization/native_serializer.h"

// Audio asset metadata (runtime native data)
struct AudioResourceData {
    std::string sourcePath;  // Relative path to audio file
    bool is3D = true;
    bool isStreaming = false;
    bool isLooping = false;
    float duration = 0.0f; // in seconds
    int sampleRate = 0;
    int channels = 0;
};

#endif
