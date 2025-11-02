#pragma once

#include "Rendering/ParticleRenderer.h"
#include "Rendering/ParticleEmitter.h"
#include "serialisation/guid.h"

struct ParticleComponent
{
	ParticleEmitterConfiguration config; // serializable
	//std::shared_ptr<Texture> texture = nullptr;
	Resource::Guid texture; 
	BlendMode blendSettings = BlendMode::Alpha;
	bool depthWrite = false;
	uint32_t renderLayer = 10;
	bool autoPlay = true;
	std::unique_ptr<ParticleEmitter> emitter = nullptr; // not serialize

	// Custom copy constructor to handle unique_ptr
	ParticleComponent() = default;
	ParticleComponent(const ParticleComponent& other)
		: config(other.config)
		, texture(other.texture)
		, blendSettings(other.blendSettings)
		, depthWrite(other.depthWrite)
		, renderLayer(other.renderLayer)
		, autoPlay(other.autoPlay)
		, emitter(nullptr) // emitter is runtime-only, not copied
	{}

	ParticleComponent& operator=(const ParticleComponent& other) {
		if (this != &other) {
			config = other.config;
			texture = other.texture;
			blendSettings = other.blendSettings;
			depthWrite = other.depthWrite;
			renderLayer = other.renderLayer;
			autoPlay = other.autoPlay;
			emitter = nullptr; // emitter is runtime-only, not copied
		}
		return *this;
	}

	// Move operations are implicitly deleted due to unique_ptr, so define them
	ParticleComponent(ParticleComponent&&) = default;
	ParticleComponent& operator=(ParticleComponent&&) = default;
};