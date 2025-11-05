#pragma once

#include "Rendering/ParticleRenderer.h"
#include "Rendering/ParticleEmitter.h"
#include <native/native.h>
#include "ecs/internal/reflection.h"

struct ParticleComponent
{
	ParticleEmitterConfiguration config; // serializable
	//std::shared_ptr<Texture> texture = nullptr;
	rp::Guid texture;
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


RegisterReflectionTypeBegin(ParticleEmitterConfiguration, "ParticleEmitterConfiguration")
	MemberRegistrationV<&ParticleEmitterConfiguration::position, "Position">,
	// Emission shape
	MemberRegistrationV<&ParticleEmitterConfiguration::emissionType, "Emission Type">,
	MemberRegistrationV<&ParticleEmitterConfiguration::emitterSize, "Emitter Size (Box)">,
	MemberRegistrationV<&ParticleEmitterConfiguration::sphereRadius, "Sphere Radius">,
	// Emission settings
	MemberRegistrationV<&ParticleEmitterConfiguration::emissionRate, "Emission Rate">,
	MemberRegistrationV<&ParticleEmitterConfiguration::maxParticles, "Max Particles">,
	MemberRegistrationV<&ParticleEmitterConfiguration::looping, "Looping">,
	MemberRegistrationV<&ParticleEmitterConfiguration::duration, "Duration">,
	// Particle lifetime
	MemberRegistrationV<&ParticleEmitterConfiguration::minLifeTime, "Min Lifetime">,
	MemberRegistrationV<&ParticleEmitterConfiguration::maxLifeTime, "Max Lifetime">,
	// Visual properties
	MemberRegistrationV<&ParticleEmitterConfiguration::startColor, "Start Color">,
	MemberRegistrationV<&ParticleEmitterConfiguration::endColor, "End Color">,
	MemberRegistrationV<&ParticleEmitterConfiguration::startSize, "Start Size">,
	MemberRegistrationV<&ParticleEmitterConfiguration::endSize, "End Size">,
	MemberRegistrationV<&ParticleEmitterConfiguration::startRotation, "Start Rotation">,
	MemberRegistrationV<&ParticleEmitterConfiguration::rotationSpeed, "Rotation Speed">,
	// Physics
	MemberRegistrationV<&ParticleEmitterConfiguration::startVelocity, "Start Velocity">,
	MemberRegistrationV<&ParticleEmitterConfiguration::velocityRandomness, "Velocity Randomness">,
	MemberRegistrationV<&ParticleEmitterConfiguration::acceleration, "Acceleration">
RegisterReflectionTypeEnd

RegisterReflectionTypeBegin(ParticleComponent, "ParticleComponent")
	MemberRegistrationV<&ParticleComponent::config, "EmitterConfig">,
	MemberRegistrationV<&ParticleComponent::texture, "Texture">,
	MemberRegistrationV<&ParticleComponent::depthWrite, "DepthWrite">,
	MemberRegistrationV<&ParticleComponent::autoPlay, "AutoPlay">,
	MemberRegistrationV<&ParticleComponent::blendSettings, "BlendSetting">,
	MemberRegistrationV<&ParticleComponent::renderLayer, "RenderLayer">
RegisterReflectionTypeEnd