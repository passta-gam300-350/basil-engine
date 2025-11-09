/******************************************************************************/
/*!
\file   ParticleSystem.h
\author Team PASSTA
		Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/07
\brief  ECS system for updating particle emitters and submitting particles to renderer

This system manages all ParticleComponent entities, updates their emitters each frame,
and submits the particle data to the SceneRenderer for GPU rendering. Handles emitter
creation, transform synchronization, and resource management.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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
	static ParticleSystem& GetInstance()
	{
		static ParticleSystem instance;
		return instance;
	}
private:
	SceneRenderer* m_SceneRenderer = nullptr;
};