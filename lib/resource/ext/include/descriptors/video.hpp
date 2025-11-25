#ifndef RESOURCE_DESCRIPTOR_VIDEO
#define RESOURCE_DESCRIPTOR_VIDEO

#include <rsc-ext/descriptor.hpp>
#include <native/video.h>

struct VideoDescriptor {
    rp::descriptor_base base;
};

inline VideoResourceData CreateVideo(VideoDescriptor const& vidDesc, std::string const& path = {}, std::string const& serializedescpath = {}) {
    // Copy data and include source path for runtime access
    VideoResourceData data{};
    std::ifstream file{ vidDesc.base.m_source, std::ios::binary | std::ios::ate };

    if (file) {
        data.m_VidData.AllocateExact(file.gcount());
        file.seekg(0, std::ios::beg);
        file.read(data.Raw(), data.Size());

        SerializeBinary(data, vidDesc.base.m_guid, ".video", path);

        // Serialize descriptor to YAML
        if (!serializedescpath.empty())
            rp::serialization::yaml_serializer::serialize(vidDesc, serializedescpath);
    }
    return data;
}

// Register audio descriptor importer
// Supported formats: .wav, .mp3, .ogg, .flac
RegisterResourceTypeImporter(VideoDescriptor, VideoResourceData, "video", ".video", CreateVideo, ".mpg")

#endif
