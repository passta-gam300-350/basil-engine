/******************************************************************************/
/*!
\file   ParticleRenderer.h
\author Team PASSTA
		Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/07
\brief  GPU-side particle renderer using instanced rendering and billboarding

This class handles GPU rendering of particle systems using instanced draw calls.
Particles are rendered as camera-facing billboards in world space. Uses SSBO
for efficient instance data upload and supports various blend modes (alpha, additive,
multiply). Integrates with the render pipeline through RenderPass submission.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once
#include "../Buffer/ShaderStorageBuffer.h"
#include "../Utility/Particle.h"
#include "../Resources/Shader.h"
#include "../Resources/Mesh.h"
#include "../Resources/Texture.h"
#include "Pipeline/RenderPass.h"
#include "Utility/FrameData.h"
#include <memory>
#include <vector>

class ParticleRenderer
{
public:
	ParticleRenderer();
	~ParticleRenderer();
    // Application submits particle systems to be rendered
	void SubmitParticleSystem(ParticleRenderData const& dataNeeded);
	void RenderToPass(RenderPass& passData, FrameData const& frameData);
	void ClearFrame();
	void SetMaxParticles(uint32_t maxParticles);
	uint32_t GetParticleCount() const;
    void SetParticleShader(const std::shared_ptr<Shader>& shader);
private:
    void InitializeResources();
    void CreateBillboardQuad();
    void EnsureWhiteTexture();
    void SetBlendMode(BlendMode mode, RenderPass& pass);

    std::shared_ptr<VertexArray> m_QuadVAO;
    std::shared_ptr<Shader> m_ParticleShader;
    std::unique_ptr<ShaderStorageBuffer> m_InstanceSSBO;
    std::vector<ParticleRenderData> m_SubmittedSystems; // different packaged data
    Texture m_WhiteTexture{}; // 1x1 white fallback for untextured particles (zero-init ensures id=0)

    uint32_t m_MaxParticles = 10000;
    uint32_t m_TotalParticleCount = 0;

    static constexpr uint32_t PARTICLE_SSBO_BINDING = 0U;
};