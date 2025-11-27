#include "../../include/Pipeline/BloomRenderPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Resources/ResourceManager.h"
#include <spdlog/spdlog.h>
#include <glad/glad.h>

BloomRenderPass::BloomRenderPass()
	: RenderPass("BloomPass")  // No base framebuffer (creates its own mip chain)
{
	//spdlog::info("BloomRenderPass: Created (will initialize on first execute)");

	// Configure custom resize logic
	SetResizeMode(ResizeMode::Custom);
}

void BloomRenderPass::Initialize(uint32_t windowWidth, uint32_t windowHeight)
{
	if (m_Initialized) return;

	m_SrcViewportSize = glm::ivec2(windowWidth, windowHeight);
	m_SrcViewportSizeFloat = glm::vec2((float)windowWidth, (float)windowHeight);

	// Create mip chain
	glm::vec2 mipSize = m_SrcViewportSizeFloat;
	glm::ivec2 mipIntSize = m_SrcViewportSize;

	for (uint32_t i = 0; i < NUM_BLOOM_MIPS; i++)
	{
		mipSize *= 0.5f;
		mipIntSize /= 2;

		// Ensure minimum size of 1x1
		if (mipIntSize.x < 1) mipIntSize.x = 1;
		if (mipIntSize.y < 1) mipIntSize.y = 1;

		// Create framebuffer for this mip using existing FrameBuffer class
		FBOSpecs mipSpec{
			static_cast<uint32_t>(mipIntSize.x),
			static_cast<uint32_t>(mipIntSize.y),
			{
				{ FBOTextureFormat::RGB11F_G11F_B10F }  // Compact HDR format
			}
		};

		BloomMip mip;
		mip.fbo = std::make_shared<FrameBuffer>(mipSpec);
		mip.size = mipSize;
		mip.intSize = mipIntSize;

		m_MipChain.push_back(mip);

		//spdlog::info("BloomRenderPass: Created mip {} ({}x{})", i, mipIntSize.x, mipIntSize.y);
	}

	// Load shaders
	m_DownsampleShader = std::make_shared<Shader>(
		"assets/shaders/bloom_downsample.vert",
		"assets/shaders/bloom_downsample.frag"
	);

	m_UpsampleShader = std::make_shared<Shader>(
		"assets/shaders/bloom_upsample.vert",
		"assets/shaders/bloom_upsample.frag"
	);

	// Set up shader uniforms
	m_DownsampleShader->use();
	m_DownsampleShader->setInt("srcTexture", 0);

	m_UpsampleShader->use();
	m_UpsampleShader->setInt("srcTexture", 0);

	glUseProgram(0);

	m_Initialized = true;
	//spdlog::info("BloomRenderPass: Initialized with {} mips", NUM_BLOOM_MIPS);
}

void BloomRenderPass::Execute(RenderContext& context)
{
	// Initialize on first execute
	if (!m_Initialized) {
		Initialize(context.frameData.viewportWidth, context.frameData.viewportHeight);
	}

	// Auto-handle resize (calls OnResize() if needed)
	CheckAndResizeIfNeeded(context);

	// Get HDR resolved buffer (output of HDRResolvePass)
	if (!context.frameData.hdrResolvedBuffer) {
		spdlog::warn("BloomRenderPass: No HDR resolved buffer available, skipping bloom");
		return;
	}

	uint32_t hdrTexture = context.frameData.hdrResolvedBuffer->GetColorAttachmentRendererID(0);

	// Begin pass (clears command buffer, no framebuffer to bind)
	Begin();

	// Setup command buffer
	SetupCommandBuffer(context);

	// Render bloom
	RenderDownsamples(hdrTexture);
	RenderUpsamples();

	// Store bloom texture in frame data for tone mapping
	context.frameData.bloomTexture = GetBloomTexture();

	// Restore viewport
	Submit(RenderCommands::SetViewportData{
		0, 0,
		context.frameData.viewportWidth,
		context.frameData.viewportHeight
	});

	ExecuteCommands();

	// End pass
	End();
}

void BloomRenderPass::RenderDownsamples(uint32_t srcTexture)
{
	// Bind downsample shader
	Submit(RenderCommands::BindShaderData{m_DownsampleShader});

	// Set initial resolution
	Submit(RenderCommands::SetUniformVec2Data{
		m_DownsampleShader,
		"srcResolution",
		m_SrcViewportSizeFloat
	});

	// Enable Karis average for first mip
	if (m_KarisAverage) {
		Submit(RenderCommands::SetUniformIntData{
			m_DownsampleShader,
			"mipLevel",
			0
		});
	}

	// Bind source texture (HDR resolved buffer)
	Submit(RenderCommands::BindTextureIDData{
		srcTexture,
		0,
		m_DownsampleShader,
		"srcTexture"
	});

	// Progressively downsample through mip chain
	for (size_t i = 0; i < m_MipChain.size(); i++)
	{
		const BloomMip& mip = m_MipChain[i];

		// Bind this mip's framebuffer
		mip.fbo->Bind();

		// Set viewport to mip resolution
		Submit(RenderCommands::SetViewportData{
			0, 0,
			static_cast<uint32_t>(mip.intSize.x),
			static_cast<uint32_t>(mip.intSize.y)
		});

		// Render fullscreen quad
		RenderQuad();

		// Execute commands for this mip
		ExecuteCommands();

		// Clear commands after execution to prevent accumulation
		ClearCommands();

		// Unbind framebuffer
		mip.fbo->Unbind();

		// Update resolution for next iteration
		Submit(RenderCommands::SetUniformVec2Data{
			m_DownsampleShader,
			"srcResolution",
			mip.size
		});

		// Bind current mip as input for next iteration
		uint32_t mipTexture = mip.fbo->GetColorAttachmentRendererID(0);
		Submit(RenderCommands::BindTextureIDData{
			mipTexture,
			0,
			m_DownsampleShader,
			"srcTexture"
		});

		// Disable Karis average after first mip
		if (i == 0 && m_KarisAverage) {
			Submit(RenderCommands::SetUniformIntData{
				m_DownsampleShader,
				"mipLevel",
				1
			});
		}
	}
}

void BloomRenderPass::RenderUpsamples()
{
	// Clear any leftover commands from downsample pass
	ClearCommands();

	// Bind upsample shader
	Submit(RenderCommands::BindShaderData{m_UpsampleShader});

	// Set filter radius
	Submit(RenderCommands::SetUniformFloatData{
		m_UpsampleShader,
		"filterRadius",
		m_FilterRadius
	});

	// Enable additive blending
	Submit(RenderCommands::SetBlendingData{
		true,           // enable
		GL_ONE,         // src factor
		GL_ONE,         // dst factor
		GL_FUNC_ADD     // equation
	});

	// Upsample from smallest to largest (reverse order)
	for (int i = (int)m_MipChain.size() - 1; i > 0; i--)
	{
		const BloomMip& mip = m_MipChain[i];        // Source (smaller)
		const BloomMip& nextMip = m_MipChain[i-1];  // Target (larger)

		// Bind source mip texture
		uint32_t srcMipTexture = mip.fbo->GetColorAttachmentRendererID(0);
		Submit(RenderCommands::BindTextureIDData{
			srcMipTexture,
			0,
			m_UpsampleShader,
			"srcTexture"
		});

		// Bind target mip framebuffer
		nextMip.fbo->Bind();

		// Set viewport to target mip resolution
		Submit(RenderCommands::SetViewportData{
			0, 0,
			static_cast<uint32_t>(nextMip.intSize.x),
			static_cast<uint32_t>(nextMip.intSize.y)
		});

		// Render fullscreen quad (will be additively blended)
		RenderQuad();

		// Execute commands for this mip
		ExecuteCommands();

		// Clear commands after execution to prevent accumulation
		ClearCommands();

		// Unbind framebuffer
		nextMip.fbo->Unbind();
	}

	// Disable blending
	Submit(RenderCommands::SetBlendingData{
		false,          // disable
		GL_ONE,
		GL_ZERO,
		GL_FUNC_ADD
	});
}

void BloomRenderPass::RenderQuad()
{
	// Initialize quad VAO if needed
	if (m_QuadVAO == 0)
	{
		float quadVertices[] = {
			// positions     // texCoords
			-1.0f,  1.0f,    0.0f, 1.0f,
			-1.0f, -1.0f,    0.0f, 0.0f,
			 1.0f,  1.0f,    1.0f, 1.0f,
			 1.0f, -1.0f,    1.0f, 0.0f,
		};

		glGenVertexArrays(1, &m_QuadVAO);
		glGenBuffers(1, &m_QuadVBO);
		glBindVertexArray(m_QuadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glBindVertexArray(0);
	}

	// Submit draw command
	Submit(RenderCommands::DrawArraysData{
		m_QuadVAO,
		4,
		GL_TRIANGLE_STRIP
	});
}

uint32_t BloomRenderPass::GetBloomTexture() const
{
	if (m_MipChain.empty()) return 0;
	return m_MipChain[0].fbo->GetColorAttachmentRendererID(0);
}

uint32_t BloomRenderPass::GetBloomMip(int index) const
{
	if (index < 0 || index >= (int)m_MipChain.size()) {
		return m_MipChain[0].fbo->GetColorAttachmentRendererID(0);
	}
	return m_MipChain[index].fbo->GetColorAttachmentRendererID(0);
}

void BloomRenderPass::OnResize(uint32_t newWidth, uint32_t newHeight)
{
	// Viewport size changed - recreate mip chain
	spdlog::info("BloomRenderPass: Viewport size changed to {}x{}, recreating mip chain", newWidth, newHeight);

	// Clear existing mip chain and reinitialize
	m_MipChain.clear();
	m_Initialized = false;
	Initialize(newWidth, newHeight);
}
