#include "Particles/ParticleSystem.h"
#include "Particles/ParticleComponent.h"
#include "components/transform.h"  
#include "Scene/SceneRenderer.h"
#include "Manager/ResourceSystem.hpp"
#include "Resources/Texture.h"
#include "Ecs/ecs.h"

void ParticleSystem::Init()
{

}

void ParticleSystem::Update(ecs::world& world, float dt)
{
	if (m_SceneRenderer == nullptr)
	{
		return;
	}
	auto& registry = world.impl.get_registry();
	auto view = registry.view<ParticleComponent, PositionComponent>();
	auto view2 = world.filter_entities<ParticleComponent, PositionComponent>();
	for (auto& eachEntity : view2)
	{
		ParticleComponent& particleComponent = eachEntity.get<ParticleComponent>();
		// implement later
	}
	view.each([&](auto entity, ParticleComponent& particleComp, PositionComponent& positionComp)
		{
			if (particleComp.emitter == nullptr)
			{
				particleComp.emitter = std::make_unique<ParticleEmitter>(particleComp.config);
				if (particleComp.autoPlay == true)
				{
					particleComp.emitter->Play();
				}
			}
			particleComp.emitter->GetConfig().position = positionComp.m_WorldPos;
			particleComp.emitter->Update(dt);
			ParticleRenderData renderData;
			renderData.particles = particleComp.emitter->GetParticles();
			renderData.texture = particleComp.texture;
			if (particleComp.texture != Resource::null_guid)
			{
				auto* texturePtr = ResourceRegistry::Instance().Get<std::shared_ptr<Texture>>(particleComp.texture);
				if (texturePtr)
				{
					renderData.texture = *texturePtr;
				}
			}
			renderData.blendMode = particleComp.blendSettings;
			renderData.depthWrite = particleComp.depthWrite;
			renderData.renderLayer = particleComp.renderLayer;
			m_SceneRenderer->SubmitParticles(renderData);
		});
}

void ParticleSystem::FixedUpdate(ecs::world& world)
{

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