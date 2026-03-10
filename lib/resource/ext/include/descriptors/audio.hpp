#ifndef RESOURCE_DESCRIPTOR_AUDIO
#define RESOURCE_DESCRIPTOR_AUDIO

#include <rsc-ext/descriptor.hpp>
#include <native/audio.h>

struct AudioDescriptor {
    rp::descriptor_base base;
    AudioResourceData audio;
};

inline AudioResourceData CreateAudio(AudioDescriptor const& audioDesc, [[maybe_unused]] std::string const& path = {}, std::string const& serializedescpath = {}) {
    // Copy data and include source path for runtime access
    AudioResourceData data = audioDesc.audio;
    data.sourcePath = audioDesc.base.m_source;

    // Serialize to binary (.audio file)
    //SerializeBinary(data, audioDesc.base.m_guid, ".audio", path);

    // Serialize descriptor to YAML
    if (!serializedescpath.empty())
        rp::serialization::yaml_serializer::serialize(audioDesc, serializedescpath);

    return data;
}

// Register audio descriptor importer
// Supported formats: .wav, .mp3, .ogg, .flac
// [HALIS] Only do .ogg
RegisterResourceTypeImporter(AudioDescriptor, AudioResourceData, "audio", ".audio", CreateAudio, ".wav", ".mp3", ".ogg", ".flac")

#endif
