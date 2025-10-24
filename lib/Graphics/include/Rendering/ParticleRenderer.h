#pragma once
#include "../Buffer/ShaderStorageBuffer.h"
#include "../Utility/Particle.h"
#include "../Resources/Shader.h"
#include "../Resources/Mesh.h"
#include <memory>
#include <vector>

// Forward declarations
class RenderPass;
struct FrameData;

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
    void RenderSystem(ParticleRenderData const& system, FrameData const& frameData);
    void UpdateSSBO(std::vector<Particle>const& particles);
    void SetBlendMode(BlendMode mode);

    std::shared_ptr<VertexArray> m_QuadVAO;
    std::shared_ptr<Shader> m_ParticleShader;
    std::unique_ptr<ShaderStorageBuffer> m_InstanceSSBO; 
    std::vector<ParticleRenderData> m_SubmittedSystems; // different packaged data

    uint32_t m_MaxParticles = 10000;
    uint32_t m_TotalParticleCount = 0;

    static constexpr uint32_t PARTICLE_SSBO_BINDING = 0;
};