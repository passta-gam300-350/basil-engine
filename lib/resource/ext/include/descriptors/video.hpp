#ifndef RESOURCE_DESCRIPTOR_VIDEO_HPP
#define RESOURCE_DESCRIPTOR_VIDEO_HPP

#include <rsc-ext/descriptor.hpp>
#include <native/video.h>

struct VideoDescriptor {
    rp::descriptor_base base;
};

inline VideoResourceData CreateVideo(VideoDescriptor const& vidDesc, std::string const& path = {}, std::string const& serializedescpath = {}) {
    // Copy data and include source path for runtime access
    VideoResourceData data{};
    std::string rpath{ rp::utility::resolve_path(vidDesc.base.m_source) };
    std::ifstream file{ rp::utility::resolve_path(vidDesc.base.m_source), std::ios::binary | std::ios::ate };

    if (file) {
        data.m_VidData.AllocateExact(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(data.m_VidData.Raw()), data.m_VidData.Size());

        //SerializeBinary(data, vidDesc.base.m_guid, ".video", path);

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
