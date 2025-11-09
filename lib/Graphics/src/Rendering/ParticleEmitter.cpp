/******************************************************************************/
/*!
\file   ParticleEmitter.cpp
\author Team PASSTA
		Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/07
\brief  Implementation of CPU-side particle emitter with physics and spawning

This file implements the ParticleEmitter which handles:
- Particle spawning with configurable emission rate and shapes
- Physics simulation (velocity, acceleration, gravity)
- Lifetime management and particle recycling
- Color/size interpolation over particle lifetime
- Random generation for position, velocity, and lifetime

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Rendering\ParticleEmitter.h"

ParticleEmitter::ParticleEmitter() : m_EmissionAccumulator(0.0f), m_ElapsedTime(0.0f), m_IsPlaying(true), m_RandomEngine(std::random_device{}()), m_Distribution(0.0f, 1.0f)
{
	m_Particles.reserve(m_Config.maxParticles);
}

ParticleEmitter::ParticleEmitter(ParticleEmitterConfiguration const& config) : m_EmissionAccumulator(0.0f), m_ElapsedTime(0.0f), m_IsPlaying(true), m_RandomEngine(std::random_device{}()), m_Distribution(0.0f, 1.0f), m_Config(config)
{
	m_Particles.reserve(m_Config.maxParticles);
}

void ParticleEmitter::Update(float deltaTime)
{
	m_ElapsedTime += deltaTime;
	EmitParticles(deltaTime);
	UpdateParticles(deltaTime);
}

void ParticleEmitter::Reset()
{
	for (auto& eachParticle : m_Particles)
	{
		eachParticle.active = false;
	}
	m_EmissionAccumulator = 0.0f;
	m_ElapsedTime = 0.0f;
	m_IsPlaying = true;
}

void ParticleEmitter::Stop()
{
	m_IsPlaying = false;
}

void ParticleEmitter::Play()
{
	m_IsPlaying = true;
}

void ParticleEmitter::SetConfig(ParticleEmitterConfiguration const& config)
{
	m_Config = config;
	m_Particles.reserve(m_Config.maxParticles);
}

ParticleEmitterConfiguration& ParticleEmitter::GetConfig()
{
	return m_Config;
}

const ParticleEmitterConfiguration& ParticleEmitter::GetConfig() const
{
	return m_Config;
}

const std::vector<Particle>& ParticleEmitter::GetParticles() const
{
	return m_Particles;
}

std::vector<Particle>& ParticleEmitter::GetParticles()
{
	return m_Particles;
}

uint32_t ParticleEmitter::GetActiveParticleCount() const
{
	uint32_t count = 0;
	for (auto const& eachParticle : m_Particles)
	{
		if (eachParticle.active)
		{
			count++;
		}
	}
	return count;
}

bool ParticleEmitter::IsPlaying() const
{
	return m_IsPlaying;
}

bool ParticleEmitter::IsAlive() const
{
	if (m_IsPlaying == true)
	{
		return true;
	}
	for (auto const& eachParticle : m_Particles)
	{
		if (eachParticle.active)
		{
			return true;
		}
	}
	return false;
}

void ParticleEmitter::EmitParticles(float deltaTime)
{
	if (m_IsPlaying == false)
	{
		return;
	}
	if (m_Config.looping == false && m_ElapsedTime >= m_Config.duration)
	{
		m_IsPlaying = false;
		return;
	}
	float particlesToEmit = m_Config.emissionRate * deltaTime;
	// the accumulator ensures we spawn the correct average rate.
	m_EmissionAccumulator += particlesToEmit;
	// integer, because emission rate can be fractional, but we cant spawn half a particle
	int count = static_cast<int>(m_EmissionAccumulator); 
	m_EmissionAccumulator -= count; // spawn 1 particle, reset accumulator
	for (int i = 0; i < count; i++)
	{
		SpawnParticle();
	}
}

void ParticleEmitter::UpdateParticles(float deltaTime)
{
	for (auto& eachParticle : m_Particles)
	{
		if (eachParticle.active == false)
		{
			continue;
		}
		eachParticle.lifeTime -= deltaTime;
		if (eachParticle.lifeTime <= 0.0f)
		{
			eachParticle.active = false;
			continue;
		}
		float lifeTimePercent = eachParticle.lifeTime / eachParticle.maxLifeTime; // calculate lifetime percentage
		eachParticle.velocity += m_Config.acceleration * deltaTime;
		eachParticle.position += eachParticle.velocity * deltaTime;
		eachParticle.rotation += m_Config.rotationSpeed * deltaTime;
		eachParticle.color = m_Config.startColor * lifeTimePercent + m_Config.endColor * (1.0f - lifeTimePercent);
		eachParticle.size = m_Config.startSize * lifeTimePercent + m_Config.endSize * (1.0f - lifeTimePercent);
	}
}

void ParticleEmitter::SpawnParticle()
{
	Particle* particle = nullptr;
	for (auto& eachParticle : m_Particles)
	{
		if (eachParticle.active == false)
		{
			particle = &eachParticle;
			break;
		}
	}
	if (particle == nullptr)
	{
		if (m_Particles.size() < static_cast<size_t>(m_Config.maxParticles))
		{
			m_Particles.emplace_back();
			particle = &m_Particles.back();
		}
		else
		{
			return;
		}
	}
	particle->position = GetRandomPositionInShape();
	particle->velocity = GetRandomVelocity();
	particle->color = m_Config.startColor;
	particle->size = m_Config.startSize;
	particle->rotation = m_Config.startRotation;
	particle->lifeTime = GetRandomLifeTime();
	particle->maxLifeTime = particle->lifeTime;
	particle->active = true;
}

glm::vec3 ParticleEmitter::GetRandomPositionInShape() const
{
	if (m_Config.emissionType == EmissionType::Box)
	{ 
		return m_Config.position + RandomInBox(m_Config.emitterSize);
	}
	else if (m_Config.emissionType == EmissionType::Point)
	{
		return m_Config.position;
	}
	else if (m_Config.emissionType == EmissionType::Sphere)
	{
		return m_Config.position + RandomInUnitSphere() * m_Config.sphereRadius;
	}
	return m_Config.position;
}

glm::vec3 ParticleEmitter::GetRandomVelocity() const
{
	return m_Config.startVelocity + RandomInUnitSphere() * m_Config.velocityRandomness;
}

float ParticleEmitter::GetRandomLifeTime() const
{
	return RandomRange(m_Config.minLifeTime, m_Config.maxLifeTime);
}

float ParticleEmitter::RandomRange(float min, float max) const
{
	return min + m_Distribution(m_RandomEngine) * (max - min);
}

glm::vec3 ParticleEmitter::RandomInUnitSphere() const
{
	glm::vec3 point;
	do 
	{
		point = glm::vec3
		(
			RandomRange(-1.0f, 1.0f),
			RandomRange(-1.0f, 1.0f),
			RandomRange(-1.0f, 1.0f)
		);
	} 
	while (glm::dot(point, point) > 1.0f); // outside sphere

	return point;
}

glm::vec3 ParticleEmitter::RandomInBox(glm::vec3 const& size) const
{
	return glm::vec3
	(
		RandomRange(-size.x * 0.5f, size.x * 0.5f),
		RandomRange(-size.y * 0.5f, size.y * 0.5f),
		RandomRange(-size.z * 0.5f, size.z * 0.5f)
	);
}
