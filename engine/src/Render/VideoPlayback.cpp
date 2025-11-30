#include "Manager/ResourceSystem.hpp"
#include "Render/VideoComponent.hpp"
#include "Render/VideoPlayback.hpp"
#include "Render/Camera.h"
#include "Render/ShaderLibrary.hpp"
#include <Rendering/HUDRenderer.h>
#include "Engine.hpp"
#include "Component/MaterialOverridesComponent.hpp"
#include "System/Audio.hpp"

#include <fmod.h>
#include <fmod.hpp>

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

struct RuntimeVideoResourceData {
	plm_t* m_Ptr;
	FMOD::Channel* m_Chl;
	FMOD::Sound* m_Sound;
	std::binary_semaphore m_StreamSync{ 1 };
};

struct RuntimeVideoData {
	VideoResourceData m_VidData;
	std::unique_ptr<RuntimeVideoResourceData> m_VidResource{};
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
	if (it == reg.end()) {
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

// Utility: convert YCbCr frame to RGB and write bottom-to-top (unflipped for OpenGL)
static inline uint8_t clamp_int(int v) {
	return v < 0 ? 0 : (v > 255) ? 255 : (uint8_t)v;
}

void plm_frame_to_rgb_unflipped(plm_frame_t* frame, uint8_t* dest, int stride) {
	int width = frame->width;
	int height = frame->height;

	// Start writing at the *last* row in dest
	for (int y = 0; y < height; ++y) {
		// Compute source row (normal order)
		int src_row = y * frame->y.width;
		// Compute destination row (flipped vertically)
		int dst_row = (height - 1 - y) * stride;

		for (int x = 0; x < width; ++x) {
			int Y = frame->y.data[src_row + x];
			int Cb = frame->cb.data[(y / 2) * frame->cb.width + (x / 2)];
			int Cr = frame->cr.data[(y / 2) * frame->cr.width + (x / 2)];

			int y_adj = (Y - 16) * 76309 >> 16;
			int cb_adj = Cb - 128;
			int cr_adj = Cr - 128;

			int r = y_adj + ((cr_adj * 104597) >> 16);
			int g = y_adj - ((cb_adj * 25674 + cr_adj * 53278) >> 16);
			int b = y_adj + ((cb_adj * 132201) >> 16);

			dest[dst_row + x * 3 + 0] = clamp_int(r);
			dest[dst_row + x * 3 + 1] = clamp_int(g);
			dest[dst_row + x * 3 + 2] = clamp_int(b);
		}
	}
}
void LogVideoAudioStreamError(FMOD_RESULT res, std::string_view flavor_text = {}, bool fatal = false) {
	if (res == FMOD_OK) return;
	if (fatal) {
		flavor_text.empty() ? spdlog::error("Video: {}", FMOD_ErrorString(res)) : spdlog::error("Video: @{} {}", flavor_text, FMOD_ErrorString(res));
		assert(res == FMOD_RESULT::FMOD_OK && "fatal error");
	}
	flavor_text.empty() ? spdlog::warn("Video: {}", FMOD_ErrorString(res)) : spdlog::warn("Video: @{} {}", flavor_text, FMOD_ErrorString(res));
}

// Decode the next frame and update the VideoComponent
std::vector<uint8_t> getNextFrame(VideoComponent& vc, RuntimeVideoData& rvd) {
	plm_t* ptr = rvd.m_VidResource->m_Ptr;
	if (!ptr || (!vc.isPlaying && !vc.loop) || vc.isPaused) {
		return std::vector<uint8_t>{};
	}
	else if (!rvd.m_VidResource->m_Chl) {
		LogVideoAudioStreamError(AudioSystem::GetInstance().GetSystem()->playSound(rvd.m_VidResource->m_Sound, 0, false, &rvd.m_VidResource->m_Chl), "PlaySound");
	}

	vc.isPlaying = true;

	// Advance playback time
	LogVideoAudioStreamError(rvd.m_VidResource->m_Chl->setFrequency(plm_get_samplerate(ptr) * vc.playbackSpeed), "SetFrequency");

	unsigned int ms = 0;
	LogVideoAudioStreamError(rvd.m_VidResource->m_Chl->getPosition(&ms, FMOD_TIMEUNIT_MS), "GetPosition");
	double audio_time = (ms / 1000.0) * vc.playbackSpeed;

	double video_time = vc.currentTime;
	double drift = audio_time - video_time;

	rvd.m_VidResource->m_StreamSync.acquire();
	//decode frames
	plm_frame_t* frame{};
	if (fabs(drift) > 2.0) {
		plm_seek(ptr, audio_time, 0);
		frame = plm_decode_video(ptr);
		video_time = plm_get_time(ptr);
	}
	else {
		while (video_time < audio_time) {
			frame = plm_decode_video(ptr);
			video_time = plm_get_time(ptr);
			if (!frame) break;
		}
	}
	vc.currentTime = video_time;

	rvd.m_VidResource->m_StreamSync.release();
	if (!frame) {
		return std::vector<uint8_t>{};
	}
	else if (plm_has_ended(ptr)) {
		plm_rewind(ptr);
		rvd.m_VidResource->m_Chl->stop();
		rvd.m_VidResource->m_Chl = nullptr;
		if (vc.loop) {
			vc.currentTime = 0;
			
			return getNextFrame(vc, rvd);
		}
		vc.isPlaying = false;
		vc.currentTime = 0;
		return std::vector<uint8_t>{};
	}

	vc.width = frame->width;
	vc.height = frame->height;

	std::vector<uint8_t> rgb(vc.width * vc.height * 3);
	plm_frame_to_rgb_unflipped(frame, rgb.data(), frame->width * 3);

	return rgb;
}

void VideoSystem::Update(ecs::world& wrld) {
	auto view = wrld.filter_entities<VideoComponent>();
	if (!Engine::GetRenderSystem().GetSceneRenderer()->GetHUDRenderer())
		return;
	glm::vec2 screenresdims{ Engine::GetRenderSystem().GetSceneRenderer()->GetHUDRenderer()->GetReferenceResolution() };
	glm::vec2 halfdims{ screenresdims * 0.5f };
	float fbaspect = CameraSystem::GetCachedViewport().x / CameraSystem::GetCachedViewport().y;

	for (auto& ent : view)
	{
		VideoComponent& vidc = ent.get<VideoComponent>();
		if (vidc.videoGuid.m_guid == rp::null_guid) {
			continue;
		}
		RuntimeVideoData& ptr = *ResourceRegistry::Instance().Get<RuntimeVideoData>(vidc.videoGuid.m_guid);
		auto rgbres{ getNextFrame(vidc, ptr) };
		if (rgbres.empty())
			continue;
		auto texguid = getGeneratedTextures(ent, vidc);
		auto& texptr = *ResourceRegistry::Instance().Get<std::shared_ptr<Texture>>(texguid);
		glTexSubImage2D(texptr->target, 0, 0, 0, vidc.width, vidc.height, GL_RGB, GL_UNSIGNED_BYTE, rgbres.data());
		if (vidc.renderFullscreen) {
			HUDElementData vidhud{};
			vidhud.anchor = HUDAnchor::BottomLeft;
			vidhud.textureID = texptr->id;
			vidhud.size = screenresdims;
			vidhud.layer = vidc.renderLayer; //bump this ahead of bg
			float aspectdiv{ vidc.aspectratio / fbaspect };
			HUDElementData bghud{};
			bghud.anchor = HUDAnchor::BottomLeft;
			//bghud.textureID = vidc.backgroundTexture.m_guid == rp::null_guid ? 0 : (*ResourceRegistry::Instance().Get<std::shared_ptr<Texture>>(texguid))->id;
			bghud.color = vidc.backgroundColor;
			bghud.layer = vidc.renderLayer + 2;
			bghud.size = screenresdims;
			switch (vidc.fullscreenMode) {
			case VideoResizeMode::Stretch: break;
			case VideoResizeMode::Fit:
				if (vidc.aspectratio > fbaspect) { //16:9 -> 4:3, reoresent 16:9 in 4:3 in 16:9
					vidhud.size.y /= aspectdiv;
				}
				else {
					vidhud.size.x *= aspectdiv;
				}
				vidhud.anchor = HUDAnchor::Center;
				vidhud.position = halfdims;
				break;
			case VideoResizeMode::Fill:
				if (vidc.aspectratio > fbaspect) {
					vidhud.size.x *= aspectdiv;
				}
				else {
					vidhud.size.y /= aspectdiv;
				}
				vidhud.anchor = HUDAnchor::Center;
				vidhud.position = halfdims;
				break;
			case VideoResizeMode::Original:
				vidhud.size = { vidc.width * aspectdiv, vidc.height / aspectdiv };
				break;
			}
			Engine::GetRenderSystem().GetSceneRenderer()->SubmitHUDElement(vidhud);
			if (vidc.bgvisible && vidc.fullscreenMode != VideoResizeMode::Stretch) {
				//Engine::GetRenderSystem().GetSceneRenderer()->SubmitHUDElement(bghud);
			}
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

FMOD_RESULT F_CALLBACK pcmsetpos_callback(
	FMOD_SOUND* sound,
	int subsound,
	unsigned int position,
	FMOD_TIMEUNIT postype
) {
	// Non-seekable stream: just accept and do nothing
	std::cout << "Seeking to position: " << position << " (ignoring as stream is non-seekable)" << std::endl;

	return FMOD_OK;
}

// Custom PCM read callback for FMOD
extern "C" FMOD_RESULT F_CALLBACK pcmReadCallback(FMOD_SOUND* sound, void* data, unsigned int datalen) {
	short* buffer = (short*)data;
	unsigned int samplesNeeded = datalen / sizeof(short);

	// Retrieve plm instance from userData
	void* ud;
	reinterpret_cast<FMOD::Sound*>(sound)->getUserData(&ud);
	RuntimeVideoResourceData* rsc = (RuntimeVideoResourceData*)ud;
	plm_t* ptr = rsc->m_Ptr;

	rsc->m_StreamSync.acquire();
	unsigned int samplesDecoded = 0;
	while (samplesDecoded < samplesNeeded) {
		plm_samples_t* samples = plm_decode_audio(ptr);
		if (!samples || samples->count <= 0) {
			// End of stream, fill rest with silence
			for (unsigned int i = samplesDecoded; i < samplesNeeded; i++) {
				buffer[i] = 0;
			}
			break;
		}

		// Convert float [-1.0, 1.0] to PCM16
		for (int i = 0; i < samples->count * 2 && samplesDecoded < samplesNeeded; i++) {
			float s = samples->interleaved[i];
			if (s > 1.0f) s = 1.0f;
			if (s < -1.0f) s = -1.0f;
			buffer[samplesDecoded++] = static_cast<short>(s * 32767.0f);
		}
	}
	rsc->m_StreamSync.release();

	return FMOD_OK;
}

RuntimeVideoData LoadVideoData(const char* data) {
	RuntimeVideoData runtime_vd{};
	runtime_vd.m_VidData = rp::serialization::serializer<"bin">::deserialize<VideoResourceData>(
		reinterpret_cast<const std::byte*>(data)
	);
	runtime_vd.m_VidResource = std::make_unique<RuntimeVideoResourceData>();
	runtime_vd.m_VidResource->m_Ptr = plm_create_with_memory(reinterpret_cast<std::uint8_t*>(runtime_vd.m_VidData.m_VidData.Raw()), runtime_vd.m_VidData.m_VidData.Size(), FALSE);
	plm_set_audio_enabled(runtime_vd.m_VidResource->m_Ptr, TRUE);
	plm_set_video_enabled(runtime_vd.m_VidResource->m_Ptr, TRUE);

	FMOD_CREATESOUNDEXINFO exinfo = { 0 };
	exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
	exinfo.numchannels = 2;
	exinfo.defaultfrequency = plm_get_samplerate(runtime_vd.m_VidResource->m_Ptr);
	exinfo.length = static_cast<uint32_t>(std::ceil(exinfo.defaultfrequency * sizeof(short) * exinfo.numchannels * plm_get_duration(runtime_vd.m_VidResource->m_Ptr)));
	exinfo.format = FMOD_SOUND_FORMAT_PCM16;
	exinfo.decodebuffersize = 8192;
	exinfo.userdata = runtime_vd.m_VidResource.get();
	exinfo.pcmreadcallback = pcmReadCallback;
	exinfo.pcmsetposcallback = pcmsetpos_callback;

	assert(AudioSystem::GetInstance().GetSystem()->createSound(0, FMOD_OPENUSER, &exinfo, &runtime_vd.m_VidResource->m_Sound) == FMOD_RESULT::FMOD_OK);

	return runtime_vd;
}

REGISTER_RESOURCE_TYPE_ALIASE(RuntimeVideoData, video,
	LoadVideoData,
	[](RuntimeVideoData& rvd) {
	if (!rvd.m_VidResource)
		return;
	if (rvd.m_VidResource->m_Ptr) {
		plm_destroy(rvd.m_VidResource->m_Ptr);
	}
	if (rvd.m_VidResource->m_Sound) {
		rvd.m_VidResource->m_Sound->release();
	}
})