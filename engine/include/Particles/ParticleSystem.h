#pragma once

#include "ParticleComponent.h"
#include "Rendering/ParticleRenderer.h"
#include "Rendering/ParticleEmitter.h"
#include "Scene/SceneRenderer.h"
#include "System/System.hpp"
#include "Ecs/ecs.h"

class SceneRenderer;

class ParticleSystem : public ecs::SystemBase
{
public:
	ParticleSystem() = default;
	~ParticleSystem() override = default;
	void Init() override;
	void Update(ecs::world& world, float dt) override;
	void FixedUpdate(ecs::world& world) override;
	void Exit() override;
	void setRenderer(SceneRenderer* renderer);
private:
	SceneRenderer* m_SceneRenderer = nullptr;
};