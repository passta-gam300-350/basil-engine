#include "Rendering/ParticleRenderer.h"
#include "Pipeline/RenderPass.h"
#include "Utility/FrameData.h"
#include "Resources/PrimitiveGenerator.h"
#include "Resources/Texture.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>


ParticleRenderer::ParticleRenderer() : m_MaxParticles(10000), m_TotalParticleCount(0)
{
	InitializeResources();
}

ParticleRenderer::~ParticleRenderer()
{
	// smart pointer auto clean up noice
}

void ParticleRenderer::SubmitParticleSystem(ParticleRenderData const& dataNeeded)
{
	m_SubmittedSystems.push_back(dataNeeded);
}

void ParticleRenderer::RenderToPass(RenderPass& pass, FrameData const& frameData)
{
	if (m_SubmittedSystems.empty() || !m_ParticleShader)
	{
		return;
	}
	m_TotalParticleCount = 0;
	for (auto const& eachSystem : m_SubmittedSystems)
	{
		if (eachSystem.particles.empty() == false)
		{
			RenderSystem(eachSystem, frameData, pass);
			m_TotalParticleCount += eachSystem.particles.size();
		}
	}
}

void ParticleRenderer::ClearFrame()
{
	m_SubmittedSystems.clear();
	m_TotalParticleCount = 0;
}

void ParticleRenderer::SetMaxParticles(uint32_t maxParticles)
{
	m_MaxParticles = maxParticles;
	size_t bufferSize = m_MaxParticles * sizeof(ParticleInstanceData);
	m_InstanceSSBO->Resize(bufferSize);
}

uint32_t ParticleRenderer::GetParticleCount() const
{
	return m_TotalParticleCount;
}

void ParticleRenderer::SetParticleShader(const std::shared_ptr<Shader>& shader)
{
	m_ParticleShader = shader;
}

void ParticleRenderer::InitializeResources()
{
	CreateBillboardQuad();
	size_t bufferSize = m_MaxParticles * sizeof(ParticleInstanceData);
	m_InstanceSSBO = std::make_unique<ShaderStorageBuffer>(nullptr, bufferSize, GL_DYNAMIC_DRAW);
	m_InstanceSSBO->BindBase(PARTICLE_SSBO_BINDING);
	// NOTE: Shader must be set via SetParticleShader() before rendering!
}

void ParticleRenderer::CreateBillboardQuad()
{
	Mesh quadMesh = PrimitiveGenerator::CreateFullscreenQuad();
	m_QuadVAO = quadMesh.GetVertexArray();
}

void ParticleRenderer::RenderSystem(ParticleRenderData const& system, FrameData const& frameData, RenderPass& pass)
{
	UpdateSSBO(system.particles);
	uint32_t activeCount = 0;
	for (auto const& eachParticle : system.particles)
	{
		if (eachParticle.active == true)
		{
			activeCount++;
		}
	}
	if (activeCount == 0)
	{
		return;
	}
	// bind shader
	pass.Submit(RenderCommands::BindShaderData{ m_ParticleShader });
	// bind ssbo
	pass.Submit(RenderCommands::BindSSBOData{m_InstanceSSBO->GetSSBOHandle(), PARTICLE_SSBO_BINDING});
	// set uniforms 
	pass.Submit(RenderCommands::SetUniformsData{ m_ParticleShader, glm::mat4(1.0f), frameData.viewMatrix, frameData.projectionMatrix, frameData.cameraPosition });
	// bind texture 
	if (system.texture)
	{
		std::vector<Texture> textures = { *system.texture };
		pass.Submit(RenderCommands::BindTexturesData{ textures, m_ParticleShader });
	}
	// set blend mode
	SetBlendMode(system.blendMode, pass);
	// set depth test
	pass.Submit(RenderCommands::SetDepthTestData{ true, GL_LESS, system.depthWrite });
	// draw instanced
	pass.Submit(RenderCommands::DrawElementsInstancedData{ m_QuadVAO->GetVAOHandle(), 6, activeCount, 0, GL_TRIANGLES });
}

void ParticleRenderer::UpdateSSBO(std::vector<Particle> const& particles)
{
	std::vector<ParticleInstanceData> instanceParticleData;
	instanceParticleData.reserve(particles.size());
	for (auto const& eachParticle : particles)
	{
		if (eachParticle.active == true)
		{
			ParticleInstanceData eachParticleData;
			eachParticleData.color = eachParticle.color;
			eachParticleData.position = eachParticle.position;
			eachParticleData.rotation = eachParticle.rotation;
			eachParticleData.size = eachParticle.size;
			eachParticleData.textureID = 0; // havent implemented texture arrays yet, no worries
			eachParticleData.padding[0] = 0;
			eachParticleData.padding[1] = 0;
			instanceParticleData.push_back(eachParticleData);
		}
	}
	if (instanceParticleData.empty() == false)
	{
		m_InstanceSSBO->SetData(instanceParticleData.data(), instanceParticleData.size() * sizeof(ParticleInstanceData));
	}
}

void ParticleRenderer::SetBlendMode(BlendMode mode, RenderPass& pass)
{
	// 5. Set blending
	if (mode == BlendMode::Alpha)
	{
		pass.Submit(RenderCommands::SetBlendingData{ true, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA });
	}
	else if (mode == BlendMode::Additive)
	{
		pass.Submit(RenderCommands::SetBlendingData{ true, GL_SRC_ALPHA, GL_ONE });
	}
	else if (mode == BlendMode::Multiply) 
	{
		pass.Submit(RenderCommands::SetBlendingData{ true, GL_DST_COLOR, GL_ZERO });
	}
	else 
	{
		pass.Submit(RenderCommands::SetBlendingData{ false });
	}
}
