#include "Bindings/ManagedParticle.hpp"

#include "ecs/ecs.h"
#include "Particles/ParticleComponent.h"
#include "Rendering/ParticleEmitter.h"

static inline void apply_config_to_runtime(ParticleComponent& pc)
{
	if (pc.emitter)
		pc.emitter->SetConfig(pc.config);
}

// ---------------- Playback ----------------
void ManagedParticle::Play(uint64_t h)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	if (pc.emitter) pc.emitter->Play();
	else pc.autoPlay = true;
}
void ManagedParticle::Stop(uint64_t h)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	if (pc.emitter) pc.emitter->Stop();
	else pc.autoPlay = false;
}
void ManagedParticle::Reset(uint64_t h)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	if (pc.emitter) pc.emitter->Reset();
}
bool ManagedParticle::IsPlaying(uint64_t h)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	return pc.emitter ? pc.emitter->IsPlaying() : pc.autoPlay;
}
bool ManagedParticle::IsAlive(uint64_t h)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	return pc.emitter ? pc.emitter->IsAlive() : false;
}
int ManagedParticle::GetActiveCount(uint64_t h)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	return pc.emitter ? static_cast<int>(pc.emitter->GetActiveParticleCount()) : 0;
}

// ---------------- Component flags ----------------
bool ManagedParticle::GetAutoPlay(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().autoPlay;
}
void ManagedParticle::SetAutoPlay(uint64_t h, bool v)
{
	ecs::entity e{ h };
	e.get<ParticleComponent>().autoPlay = v;
}
int ManagedParticle::GetBlendMode(uint64_t h)
{
	ecs::entity e{ h };
	return static_cast<int>(e.get<ParticleComponent>().blendSettings);
}
void ManagedParticle::SetBlendMode(uint64_t h, int v)
{
	ecs::entity e{ h };
	e.get<ParticleComponent>().blendSettings = static_cast<BlendMode>(v);
}
bool ManagedParticle::GetDepthWrite(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().depthWrite;
}
void ManagedParticle::SetDepthWrite(uint64_t h, bool v)
{
	ecs::entity e{ h };
	e.get<ParticleComponent>().depthWrite = v;
}
uint32_t ManagedParticle::GetRenderLayer(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().renderLayer;
}
void ManagedParticle::SetRenderLayer(uint64_t h, uint32_t v)
{
	ecs::entity e{ h };
	e.get<ParticleComponent>().renderLayer = v;
}

// ---------------- Emitter config: Emission shape ----------------
int ManagedParticle::GetEmissionType(uint64_t h)
{
	ecs::entity e{ h };
	return static_cast<int>(e.get<ParticleComponent>().config.emissionType);
}
void ManagedParticle::SetEmissionType(uint64_t h, int v)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.emissionType = static_cast<EmissionType>(v);
	apply_config_to_runtime(pc);
}
float ManagedParticle::GetEmitterSizeX(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().config.emitterSize.x;
}
float ManagedParticle::GetEmitterSizeY(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().config.emitterSize.y;
}
float ManagedParticle::GetEmitterSizeZ(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().config.emitterSize.z;
}
void ManagedParticle::SetEmitterSize(uint64_t h, float x, float y, float z)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.emitterSize = { x, y, z };
	apply_config_to_runtime(pc);
}
float ManagedParticle::GetSphereRadius(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().config.sphereRadius;
}
void ManagedParticle::SetSphereRadius(uint64_t h, float r)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.sphereRadius = r;
	apply_config_to_runtime(pc);
}

// ---------------- Emitter config: Emission settings ----------------
float ManagedParticle::GetEmissionRate(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().config.emissionRate;
}
void ManagedParticle::SetEmissionRate(uint64_t h, float v)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.emissionRate = v;
	apply_config_to_runtime(pc);
}
int ManagedParticle::GetMaxParticles(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().config.maxParticles;
}
void ManagedParticle::SetMaxParticles(uint64_t h, int v)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.maxParticles = v;
	apply_config_to_runtime(pc);
}
bool ManagedParticle::GetLooping(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().config.looping;
}
void ManagedParticle::SetLooping(uint64_t h, bool v)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.looping = v;
	apply_config_to_runtime(pc);
}
float ManagedParticle::GetDuration(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().config.duration;
}
void ManagedParticle::SetDuration(uint64_t h, float v)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.duration = v;
	apply_config_to_runtime(pc);
}

// ---------------- Lifetime ----------------
float ManagedParticle::GetMinLifetime(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().config.minLifeTime;
}
void ManagedParticle::SetMinLifetime(uint64_t h, float v)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.minLifeTime = v;
	apply_config_to_runtime(pc);
}
float ManagedParticle::GetMaxLifetime(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().config.maxLifeTime;
}
void ManagedParticle::SetMaxLifetime(uint64_t h, float v)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.maxLifeTime = v;
	apply_config_to_runtime(pc);
}

// ---------------- Visuals ----------------
float ManagedParticle::GetStartColorR(uint64_t h)
{
	ecs::entity e{ h }; return e.get<ParticleComponent>().config.startColor.r;
}
float ManagedParticle::GetStartColorG(uint64_t h)
{
	ecs::entity e{ h }; return e.get<ParticleComponent>().config.startColor.g;
}
float ManagedParticle::GetStartColorB(uint64_t h)
{
	ecs::entity e{ h }; return e.get<ParticleComponent>().config.startColor.b;
}
float ManagedParticle::GetStartColorA(uint64_t h)
{
	ecs::entity e{ h }; return e.get<ParticleComponent>().config.startColor.a;
}
void ManagedParticle::SetStartColor(uint64_t h, float r, float g, float b, float a)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.startColor = { r, g, b, a };
	apply_config_to_runtime(pc);
}
float ManagedParticle::GetEndColorR(uint64_t h)
{
	ecs::entity e{ h }; return e.get<ParticleComponent>().config.endColor.r;
}
float ManagedParticle::GetEndColorG(uint64_t h)
{
	ecs::entity e{ h }; return e.get<ParticleComponent>().config.endColor.g;
}
float ManagedParticle::GetEndColorB(uint64_t h)
{
	ecs::entity e{ h }; return e.get<ParticleComponent>().config.endColor.b;
}
float ManagedParticle::GetEndColorA(uint64_t h)
{
	ecs::entity e{ h }; return e.get<ParticleComponent>().config.endColor.a;
}
void ManagedParticle::SetEndColor(uint64_t h, float r, float g, float b, float a)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.endColor = { r, g, b, a };
	apply_config_to_runtime(pc);
}
float ManagedParticle::GetStartSize(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().config.startSize;
}
void ManagedParticle::SetStartSize(uint64_t h, float v)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.startSize = v;
	apply_config_to_runtime(pc);
}
float ManagedParticle::GetEndSize(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().config.endSize;
}
void ManagedParticle::SetEndSize(uint64_t h, float v)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.endSize = v;
	apply_config_to_runtime(pc);
}
float ManagedParticle::GetStartRotation(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().config.startRotation;
}
void ManagedParticle::SetStartRotation(uint64_t h, float v)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.startRotation = v;
	apply_config_to_runtime(pc);
}
float ManagedParticle::GetRotationSpeed(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().config.rotationSpeed;
}
void ManagedParticle::SetRotationSpeed(uint64_t h, float v)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.rotationSpeed = v;
	apply_config_to_runtime(pc);
}

// ---------------- Physics ----------------
float ManagedParticle::GetStartVelX(uint64_t h)
{
	ecs::entity e{ h }; return e.get<ParticleComponent>().config.startVelocity.x;
}
float ManagedParticle::GetStartVelY(uint64_t h)
{
	ecs::entity e{ h }; return e.get<ParticleComponent>().config.startVelocity.y;
}
float ManagedParticle::GetStartVelZ(uint64_t h)
{
	ecs::entity e{ h }; return e.get<ParticleComponent>().config.startVelocity.z;
}
void ManagedParticle::SetStartVelocity(uint64_t h, float x, float y, float z)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.startVelocity = { x, y, z };
	apply_config_to_runtime(pc);
}
float ManagedParticle::GetVelocityRandomness(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<ParticleComponent>().config.velocityRandomness;
}
void ManagedParticle::SetVelocityRandomness(uint64_t h, float v)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.velocityRandomness = v;
	apply_config_to_runtime(pc);
}
float ManagedParticle::GetAccelX(uint64_t h)
{
	ecs::entity e{ h }; return e.get<ParticleComponent>().config.acceleration.x;
}
float ManagedParticle::GetAccelY(uint64_t h)
{
	ecs::entity e{ h }; return e.get<ParticleComponent>().config.acceleration.y;
}
float ManagedParticle::GetAccelZ(uint64_t h)
{
	ecs::entity e{ h }; return e.get<ParticleComponent>().config.acceleration.z;
}
void ManagedParticle::SetAcceleration(uint64_t h, float x, float y, float z)
{
	ecs::entity e{ h };
	auto& pc = e.get<ParticleComponent>();
	pc.config.acceleration = { x, y, z };
	apply_config_to_runtime(pc);
}
