#pragma once

#include "Rendering/ParticleRenderer.h"
#include "Rendering/ParticleEmitter.h"

struct ParticleComponent
{
	ParticleEmitterConfiguration config; // serializable
	std::shared_ptr<Texture> texture = nullptr;
	BlendMode blendSettings = BlendMode::Alpha;
	bool depthWrite = false;
	uint32_t renderLayer = 10;
	bool autoPlay = true;
	std::unique_ptr<ParticleEmitter> emitter = nullptr; // not serialize
};