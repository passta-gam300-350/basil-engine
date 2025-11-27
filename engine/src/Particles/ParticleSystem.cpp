/******************************************************************************/
/*!
\file   ParticleSystem.cpp
\author Team PASSTA
		Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/07
\brief  Implementation of ECS particle system update and submission logic

This file implements the ParticleSystem which manages ParticleComponent entities.
Each frame, it updates particle emitters, synchronizes them with entity transforms,
and submits particle data to the SceneRenderer for GPU rendering.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleComponent.h"
#include "components/transform.h"
#include "Scene/SceneRenderer.h"
#include "Ecs/ecs.h"
#include "Manager/ResourceSystem.hpp"
#include "Resources/Texture.h"
#include "Profiler/profiler.hpp"

void ParticleSystem::Init()
{
	
}

void ParticleSystem::Update(ecs::world& world, float dt)
{
	PF_SYSTEM("ParticleSystem");
	if (m_SceneRenderer == nullptr)
	{
		return;
	}
	//auto& registry = world.impl.get_registry();
	//auto view = registry.view<ParticleComponent, TransformComponent>();
	auto view = world.filter_entities<ParticleComponent, TransformComponent>();

	for (auto& eachEntity : view)
	{
		ParticleComponent& particleComponent = eachEntity.get<ParticleComponent>();
		TransformComponent& transformComponent = eachEntity.get<TransformComponent>();
		if (particleComponent.emitter == nullptr)
		{
			particleComponent.emitter = std::make_unique<ParticleEmitter>(particleComponent.config);
			if (particleComponent.autoPlay == true)
			{
				particleComponent.emitter->Play();
			}
		}
		particleComponent.emitter->GetConfig().position = transformComponent.m_Translation;
		particleComponent.emitter->Update(dt);
		ParticleRenderData renderData;
		renderData.particles = particleComponent.emitter->GetParticles();

		if (particleComponent.texture != rp::null_guid)
		{
			auto* texturePtr = ResourceRegistry::Instance().Get<std::shared_ptr<Texture>>(particleComponent.texture);
			if (texturePtr)
			{
				renderData.texture = *texturePtr;
			}
		}

		renderData.blendMode = particleComponent.blendSettings;
		renderData.depthWrite = particleComponent.depthWrite;
		renderData.renderLayer = particleComponent.renderLayer;

		m_SceneRenderer->SubmitParticles(renderData);
	}
}

void ParticleSystem::FixedUpdate(ecs::world& world)
{
	Update(world, 1 / 60.f);
}

void ParticleSystem::Exit()
{
	m_SceneRenderer = nullptr;
}

void ParticleSystem::setRenderer(SceneRenderer* renderer)
{
	m_SceneRenderer = renderer;
}

RegisterSystemDerived
(
	ParticleSystem,
	ParticleSystem,
	(ecs::ReadSet<PositionComponent>),
	(ecs::WriteSet<ParticleComponent>),
	60.0f
);