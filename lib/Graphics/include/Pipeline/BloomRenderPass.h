#pragma once

#include "RenderPass.h"
#include "../Buffer/FrameBuffer.h"
#include "../Resources/Shader.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

/**
 * Physically Based Bloom Render Pass
 *
 * Implements the Call of Duty bloom method (ACM Siggraph 2014):
 * - 13-tap downsample with Karis average (firefly prevention)
 * - 9-tap tent filter upsample with additive blending
 * - Multi-scale blur (6 mip levels by default)
 * - Energy-preserving filters
 *
 * Takes HDR color buffer as input, outputs bloom texture for tone mapping.
 */
class BloomRenderPass : public RenderPass {
public:
	BloomRenderPass();
	~BloomRenderPass() = default;

	// RenderPass interface
	void Execute(RenderContext& context) override;

	// Configuration
	void SetFilterRadius(float radius) { m_FilterRadius = radius; }
	void SetBloomStrength(float strength) { m_BloomStrength = strength; }
	void SetKarisAverageEnabled(bool enabled) { m_KarisAverage = enabled; }

	// Get the final bloom texture for tone mapping
	uint32_t GetBloomTexture() const;
	uint32_t GetBloomMip(int index) const;

private:
	// Bloom mip chain
	struct BloomMip {
		std::shared_ptr<FrameBuffer> fbo;
		glm::vec2 size;
		glm::ivec2 intSize;
	};

	std::vector<BloomMip> m_MipChain;

	// Shaders
	std::shared_ptr<Shader> m_DownsampleShader;
	std::shared_ptr<Shader> m_UpsampleShader;

	// Configuration
	static constexpr uint32_t NUM_BLOOM_MIPS = 6;
	float m_FilterRadius = 0.005f;
	float m_BloomStrength = 0.04f;
	bool m_KarisAverage = true;
	bool m_Initialized = false;

	// Source viewport (from context)
	glm::ivec2 m_SrcViewportSize;
	glm::vec2 m_SrcViewportSizeFloat;

	// Initialization
	void Initialize(uint32_t windowWidth, uint32_t windowHeight);

	// Custom resize handling (override from RenderPass)
	void OnResize(uint32_t newWidth, uint32_t newHeight) override;

	// Rendering
	void RenderDownsamples(uint32_t srcTexture);
	void RenderUpsamples();

	// Fullscreen quad rendering
	void RenderQuad();
	uint32_t m_QuadVAO = 0;
	uint32_t m_QuadVBO = 0;
};
