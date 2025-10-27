#pragma once

#include "../Utility/Particle.h"
#include <glm/glm.hpp>
#include <vector>
#include <random>

enum class EmissionType
{
	Point,
	Box,
	Sphere,
	Cone
};

struct ParticleEmitterConfiguration
{
	glm::vec3 position;
	glm::vec3 direction;

	EmissionType emissionType;
	glm::vec3 emissionSize;
	float sphereRadius;
	float coneAngle;

	float emissionRate = 10.0f;
	int maxParticles = 10000;
	bool looping = true;
	float duration = 5.0f;

	float minLifeTime = 1.0f;
	float maxLifeTime = 3.0f;

	glm::vec4 startColor = glm::vec4(1.0f);
	glm::vec4 endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
	float startSize = 1.0f;
	float startRotation = 0.0f;
	float rotationSpeed = 0.0f;

	glm::vec3 startVelocity = glm::vec3(1.0f);
	float velocityRandomness = 0.5f;
	glm::vec3 acceleration = glm::vec3(0.0f, -9.8f, 0.0f);

};

class ParticleEmitter
{
public:
	ParticleEmitter();
	explicit ParticleEmitter(ParticleEmitterConfiguration const& config);
	~ParticleEmitter() = default;
	void Update(float deltaTime);
	void Reset();
	void Stop();
	void Play();
	void SetConfig(ParticleEmitterConfiguration const& config);
	ParticleEmitterConfiguration& GetConfig();
	const ParticleEmitterConfiguration& GetConfig() const;
	const std::vector<Particle>&GetParticles() const;
	std::vector<Particle>&GetParticles();
	uint32_t GetActiveParticleCount() const;
	bool IsPlaying() const;
	bool IsAlive() const;

private:
	void EmitParticles(float deltaTime);
	void UpdateParticles(float deltaTime);
	void SpawnParticle();
	glm::vec3 GetRandomPositionInShape() const;
	glm::vec3 GetRandomVelocity() const;
	float GetRandomLifeTime() const;
	float RandomRange(float min, float max) const;
	glm::vec3 RandomInUnitSphere() const;
	glm::vec3 RandomInBox(glm::vec3 const& size) const;

	// Members
	ParticleEmitterConfiguration m_Config;
	std::vector<Particle> m_Particles;
	float m_EmissionAccumulator = 0.0f;
	float m_ElapsedTime = 0.0f;
	bool m_IsPlaying = true;
	mutable std::mt19937 m_RandomEngine;
	mutable std::uniform_real_distribution<float> m_Distribution;
};
