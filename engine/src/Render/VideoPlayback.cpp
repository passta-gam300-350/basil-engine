#include "Manager/ResourceSystem.hpp"
#include "Render/VideoComponent.hpp"
#include "Render/VideoPlayback.hpp"
#include "Render/Render.h"
#include "Render/Camera.h"
#include "Render/ShaderLibrary.hpp"
#include "Engine.hpp"
#include "Component/MaterialOverridesComponent.hpp"

//#include <cstdio>
#define PL_MPEG_IMPLEMENTATION
#include <pl_mpeg.h>

struct GeneratedTextureData {
    rp::Guid guid;
    std::uint32_t width;
    std::uint32_t height;
    bool markActive;
};

std::unordered_map<std::uint64_t, GeneratedTextureData>& generatedEntityVideoTextures() {
    static std::unordered_map<std::uint64_t, GeneratedTextureData> textures{};
    return textures;
}

struct RuntimeVideoData {
    VideoResourceData m_VidData;
    plm_t* m_Ptr;
};

void removeStaleGeneratedTextures() {
    auto& reg = generatedEntityVideoTextures();
    for (auto it = reg.begin(); it != reg.end(); ) {
        if (!it->second.markActive) {
            it = reg.erase(it);  // safe, erase returns next iterator
        }
        else {
            it->second.markActive = false;
            ++it;
        }
    }
}

void removeGeneratedTextures(std::unordered_map<std::uint64_t, GeneratedTextureData>::const_iterator it) {
    auto& reg = generatedEntityVideoTextures();
    if (it->second.guid != rp::null_guid)
        ResourceRegistry::Instance().UnregisterInMemory<std::shared_ptr<Texture>>(it->second.guid);
    reg.erase(it);
}

void removeGeneratedTextures(ecs::entity ent) {
    auto& reg = generatedEntityVideoTextures();
    if (auto it = reg.find(ent.get_uuid()); it != reg.end()) {
        removeGeneratedTextures(it);
    }
}

rp::Guid getGeneratedTextures(ecs::entity ent, VideoComponent& vc) {
    auto& reg = generatedEntityVideoTextures();
    auto it = reg.find(ent.get_uuid());
    if (it != reg.end() && (it->second.width != vc.width || it->second.height != vc.height)) {
        removeGeneratedTextures(it);
    }
    if (it == reg.end()){
        GeneratedTextureData texdata{};
        texdata.height = vc.height;
        texdata.width = vc.width;
        vc.aspectratio = static_cast<float>(texdata.width) / texdata.height;
        texdata.guid = rp::Guid::generate();
        TextureData texldrdata;
        texldrdata.pixels = new unsigned char[vc.height * vc.width * 3] {};
        texldrdata.width = vc.width;
        texldrdata.channels = 3;
        texldrdata.isValid = true;
        texldrdata.height = vc.height;
        Texture tex;
        tex.id = TextureLoader::CreateGPUTexture(texldrdata);
        tex.type = "texture_diffuse";
        ResourceRegistry::Instance().RegisterInMemory<std::shared_ptr<Texture>>(texdata.guid, std::make_shared<Texture>(tex));
        it = reg.emplace(ent.get_uuid(), texdata).first;
    }
    it->second.markActive = true;
    return it->second.guid;
}

bool videoPlaybackUpdate(VideoComponent& vc, plm_t* ptr) {
    if (!ptr || (!vc.isPlaying && !vc.loop) || vc.isPaused) {
        return false;
    }
    vc.isPlaying = true;
    return true;
}

// Decode the next frame and update the VideoComponent
std::optional<std::vector<uint8_t>> getNextFrame(VideoComponent& vc, plm_t* ptr) {
    if (!ptr || (!vc.isPlaying && !vc.loop) || vc.isPaused) {
        return std::nullopt;
    }

    vc.isPlaying = true;

    // Advance playback time
    vc.currentTime += static_cast<float>(Engine::GetDeltaTime()) * vc.playbackSpeed;

    // Seek decoder to current time (skip frames if needed)
    plm_seek(ptr, vc.currentTime, /*seek_exact=*/0);

    // Decode frame
    plm_frame_t* frame = plm_decode_video(ptr);
    if (!frame) {
        if (vc.loop&&false) {
            plm_rewind(ptr);
            vc.currentTime = 0;
            return getNextFrame(vc, ptr);
        }
        else {
            vc.isPlaying = false;
            vc.currentTime = 0;
            return std::nullopt;
        }
    }

    vc.width = frame->width;
    vc.height = frame->height;

    std::vector<uint8_t> rgb(vc.width * vc.height * 3);
    plm_frame_to_rgb(frame, rgb.data(), frame->width * 3);

    // Upload YCbCr planes to OpenGL texture
    // Simplest approach: convert to RGB and upload
    /*std::vector<uint8_t> rgb(vc.width * vc.height * 3);

    for (int y = 0; y < vc.height; ++y) {
        for (int x = 0; x < vc.width; ++x) {
            int Y = frame->y.data[y * frame->width + x];
            int Cb = frame->cb.data[(y / 2) * (frame->width / 2) + (x / 2)];
            int Cr = frame->cr.data[(y / 2) * (frame->width / 2) + (x / 2)];

            int R = (int)(Y + 1.402f * (Cr - 128));
            int G = (int)(Y - 0.344136f * (Cb - 128) - 0.714136f * (Cr - 128));
            int B = (int)(Y + 1.772f * (Cb - 128));

            // Clamp
            R = std::max(0, std::min(255, R));
            G = std::max(0, std::min(255, G));
            B = std::max(0, std::min(255, B));

            int idx = (y * vc.width + x) * 3;
            rgb[idx + 0] = (uint8_t)R;
            rgb[idx + 1] = (uint8_t)G;
            rgb[idx + 2] = (uint8_t)B;
        }
    }*/

    //vc.currentTime += static_cast<float>(Engine::GetDeltaTime()) * vc.playbackSpeed;

    return std::make_optional(rgb);
}

void VideoSystem::Update(ecs::world& wrld) {
	auto view = wrld.filter_entities<VideoComponent>();
    if (!Engine::GetRenderSystem().GetSceneRenderer()->GetFrameData().mainColorBuffer)
        return;
    glm::vec2 maincolorfbdims 
    { Engine::GetRenderSystem().GetSceneRenderer()->GetFrameData().mainColorBuffer->GetSpecification().Width,
     Engine::GetRenderSystem().GetSceneRenderer()->GetFrameData().mainColorBuffer->GetSpecification().Height };
    float fbaspect = /*maincolorfbdims.x / maincolorfbdims.y;*/ CameraSystem::GetCachedViewport().x / CameraSystem::GetCachedViewport().y;

    for (auto& ent : view)
    {
        VideoComponent& vidc = ent.get<VideoComponent>();
        if (vidc.videoGuid.m_guid == rp::null_guid) {
            continue;
        }
        plm_t* ptr = ResourceRegistry::Instance().Get<RuntimeVideoData>(vidc.videoGuid.m_guid)->m_Ptr;
        auto rgbres{getNextFrame(vidc, ptr)};
        if (!rgbres)
            continue;
        auto texguid = getGeneratedTextures(ent, vidc);
        auto& texptr = *ResourceRegistry::Instance().Get<std::shared_ptr<Texture>>(texguid);
        glTexSubImage2D(texptr->target, 0, 0, 0, vidc.width, vidc.height, GL_RGB, GL_UNSIGNED_BYTE, rgbres->data());
        if (vidc.renderFullscreen) {
            HUDElementData vidhud{};
            vidhud.anchor = HUDAnchor::TopLeft;
            vidhud.textureID = texptr->id;
            vidhud.size = maincolorfbdims;
            switch (vidc.fullscreenMode) {
            case VideoResizeMode::Stretch: break;
            case VideoResizeMode::Fit:
                if (vidc.aspectratio > fbaspect) { 
                    vidhud.size.y = vidhud.size.x / vidc.aspectratio;
                }
                else { 
                    vidhud.size.x = vidhud.size.y * vidc.aspectratio; }
                break;
            case VideoResizeMode::Fill:
                if (vidc.aspectratio > fbaspect) { 
                    vidhud.size.x = vidhud.size.y * vidc.aspectratio; 
                }
                else { 
                    vidhud.size.y = vidhud.size.x / vidc.aspectratio;
                }
                break;
            case VideoResizeMode::Original:
                vidhud.size = { vidc.width, vidc.height };
                break;
            }
            Engine::GetRenderSystem().GetSceneRenderer()->SubmitHUDElement(vidhud);
        }
        else if (ent.all<MeshRendererComponent>()) { //wip
            MeshRendererComponent& meshc = ent.get<MeshRendererComponent>();
            for (auto& [matname, matguid] : meshc.m_MaterialGuid) {
                //matguid.m_guid = texguid;
            }
        }
    }
}

void VideoSystem::FixedUpdate(ecs::world&) {

}

//[](const char* data) -> plm_t* {
//    VideoResourceData vidData = rp::serialization::serializer<"bin">::deserialize<VideoResourceData>(
//        reinterpret_cast<const std::byte*>(data)
//    );
//    plm_t* ptr = plm_create_with_memory(reinterpret_cast<std::uint8_t*>(vidData.m_VidData.Raw()), vidData.m_VidData.Size(), 0);
//    plm_set_audio_enabled(ptr, TRUE);
//    plm_set_video_enabled(ptr, TRUE);
//    return ptr;
//    }

RuntimeVideoData LoadVideoData(const char* data) {
    RuntimeVideoData runtime_vd{};
    runtime_vd.m_VidData = rp::serialization::serializer<"bin">::deserialize<VideoResourceData>(
        reinterpret_cast<const std::byte*>(data)
    );
    runtime_vd.m_Ptr = plm_create_with_memory(reinterpret_cast<std::uint8_t*>(runtime_vd.m_VidData.m_VidData.Raw()), runtime_vd.m_VidData.m_VidData.Size(), FALSE);
    plm_set_audio_enabled(runtime_vd.m_Ptr, TRUE);
    plm_set_video_enabled(runtime_vd.m_Ptr, TRUE);
    return runtime_vd;
    }

REGISTER_RESOURCE_TYPE_ALIASE(RuntimeVideoData, video,
    LoadVideoData,
    [](RuntimeVideoData& rvd) {
        if (rvd.m_Ptr) {
            plm_destroy(rvd.m_Ptr);
        }
    })