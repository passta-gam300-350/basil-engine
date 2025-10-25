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

void ParticleRenderer::RenderToPass(RenderPass& passData, FrameData const& frameData)
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
			RenderSystem(eachSystem, frameData);
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

void ParticleRenderer::RenderSystem(ParticleRenderData const& system, FrameData const& frameData)
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
	m_ParticleShader->use();
	m_ParticleShader->setMat4("uProjection", frameData.projectionMatrix);
	m_ParticleShader->setMat4("uView", frameData.viewMatrix);
	if (system.texture)
	{
		glActiveTexture(GL_TEXTURE0);  
		glBindTexture(GL_TEXTURE_2D, system.texture->id); 
		m_ParticleShader->setInt("uTexture", 0);  
	}
	SetBlendMode(system.blendMode);
	if (system.depthWrite == false) 
	{
		glDepthMask(GL_FALSE);
	}
	m_QuadVAO->Bind();
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, activeCount);
	if (system.depthWrite == false)
	{
		glDepthMask(GL_TRUE);
	}
	glDisable(GL_BLEND);
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

void ParticleRenderer::SetBlendMode(BlendMode mode)
{
	glEnable(GL_BLEND);
	if (mode == BlendMode::Alpha)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else if (mode == BlendMode::Additive)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}
	else if (mode == BlendMode::Multiply)
	{
		glBlendFunc(GL_DST_COLOR, GL_ZERO);
	}
	else if (mode == BlendMode::Opaque)
	{
		glDisable(GL_BLEND);
	}
}
