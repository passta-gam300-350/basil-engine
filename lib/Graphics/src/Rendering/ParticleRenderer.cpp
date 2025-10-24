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
	
}

void ParticleRenderer::RenderSystem(ParticleRenderData const& system, FrameData const& frameData)
{
}

void ParticleRenderer::UpdateSSBO(std::vector<Particle> const& particles)
{
}

void ParticleRenderer::SetBlendMode(BlendMode mode)
{
}
