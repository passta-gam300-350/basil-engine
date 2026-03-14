/******************************************************************************/
/*!
\file   ParticleRenderer.cpp
\author Team PASSTA
		Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/07
\brief  Implementation of GPU-side particle renderer with instanced billboards

This file implements the ParticleRenderer which handles:
- Billboard quad generation in XY plane for camera-facing rendering
- SSBO upload of particle instance data (position, size, color, rotation)
- Instanced draw calls for efficient GPU rendering
- Blend mode configuration (alpha, additive, multiply)
- Integration with render pipeline through command submission

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Rendering/ParticleRenderer.h"
#include "Pipeline/RenderPass.h"
#include "Utility/FrameData.h"
#include "Resources/PrimitiveGenerator.h"
#include "Resources/Texture.h"
#include <spdlog/spdlog.h>
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
			m_TotalParticleCount += static_cast<uint32_t>(eachSystem.particles.size());
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
	m_InstanceSSBO->Resize(static_cast<uint32_t>(bufferSize));
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
	m_InstanceSSBO = std::make_unique<ShaderStorageBuffer>(nullptr, uint32_t(bufferSize), GL_DYNAMIC_DRAW);
	m_InstanceSSBO->BindBase(PARTICLE_SSBO_BINDING);

	// NOTE: Shader must be set via SetParticleShader() before rendering!
	// White fallback texture is created lazily on first render (EnsureWhiteTexture)
}

void ParticleRenderer::CreateBillboardQuad()
{
	// Create a centered quad in XY plane (for billboarding)
	// The particle shader uses aPosition.x and aPosition.y for billboard rotation
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	// Create 4 vertices for a 1x1 quad centered at origin in XY plane
	Vertex v;
	for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) 
	{
		v.m_BoneIDs[i] = -1;
		v.m_Weights[i] = 0.0f;
	}
	v.Normal = glm::vec3(0.0f, 0.0f, 1.0f);  // Facing forward
	v.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
	v.Bitangent = glm::vec3(0.0f, 1.0f, 0.0f);

	// Bottom-left (-0.5, -0.5, 0)
	v.Position = glm::vec3(-0.5f, -0.5f, 0.0f);
	v.TexCoords = glm::vec2(0.0f, 0.0f);
	vertices.push_back(v);

	// Bottom-right (0.5, -0.5, 0)
	v.Position = glm::vec3(0.5f, -0.5f, 0.0f);
	v.TexCoords = glm::vec2(1.0f, 0.0f);
	vertices.push_back(v);

	// Top-right (0.5, 0.5, 0)
	v.Position = glm::vec3(0.5f, 0.5f, 0.0f);
	v.TexCoords = glm::vec2(1.0f, 1.0f);
	vertices.push_back(v);

	// Top-left (-0.5, 0.5, 0)
	v.Position = glm::vec3(-0.5f, 0.5f, 0.0f);
	v.TexCoords = glm::vec2(0.0f, 1.0f);
	vertices.push_back(v);

	// Two triangles (counter-clockwise winding)
	indices = { 0, 1, 2, 0, 2, 3 };

	Mesh quadMesh(vertices, indices, {});
	m_QuadVAO = quadMesh.GetVertexArray();
}

void ParticleRenderer::EnsureWhiteTexture()
{
	if (m_WhiteTexture.id != 0)
	{
		return;
	}
	unsigned int whiteTexID = 0;
	glGenTextures(1, &whiteTexID);
	glBindTexture(GL_TEXTURE_2D, whiteTexID);
	unsigned char whitePixel[4] = { 255, 255, 255, 255 };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	m_WhiteTexture = Texture{ whiteTexID, "texture_diffuse", "white", GL_TEXTURE_2D };
}

void ParticleRenderer::RenderSystem(ParticleRenderData const& system, FrameData const& frameData, RenderPass& pass)
{
	EnsureWhiteTexture();
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
	// bind texture: use real texture if available, otherwise white fallback (white * color = color)
	uint32_t texID = (system.texture && system.texture->id != 0) ? system.texture->id : m_WhiteTexture.id;
	pass.Submit(RenderCommands::BindTextureIDData{ texID, 0, m_ParticleShader, "uTexture" });
	// set blend mode
	SetBlendMode(system.blendMode, pass);
	// set depth test
	pass.Submit(RenderCommands::SetDepthTestData{ true, GL_LESS, false });
	// draw instanced
	uint32_t vaoHandle = m_QuadVAO->GetVAOHandle();
	pass.Submit(RenderCommands::DrawElementsInstancedData{ vaoHandle, 6, activeCount, 0, GL_TRIANGLES });
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
		m_InstanceSSBO->SetData(instanceParticleData.data(), static_cast<uint32_t>(instanceParticleData.size() * sizeof(ParticleInstanceData)));
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
