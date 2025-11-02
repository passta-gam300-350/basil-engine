#include "Pipeline/ToneMapRenderPass.h"
#include "Pipeline/RenderContext.h"
#include <spdlog/spdlog.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>

ToneMapRenderPass::ToneMapRenderPass()
    : RenderPass("ToneMapPass", FBOSpecs{
        1280, 720,
        {
            { FBOTextureFormat::RGB8 },  // RGB8 output with manual gamma correction in shader
        },
        1  // No MSAA for post-process
    })
{
    CreateFullScreenQuad();

    // Configure auto-resize to match viewport
    SetResizeMode(ResizeMode::MatchViewport);
}

void ToneMapRenderPass::Execute(RenderContext& context)
{
    if (!m_ToneMappingShader) {
        spdlog::warn("ToneMapRenderPass: No tone mapping shader set!");
        return;
    }

    // Auto-resize framebuffer to match viewport if needed
    CheckAndResizeIfNeeded(context);

    // Disable GL_FRAMEBUFFER_SRGB since we're using manual gamma correction
    // (RGB8 output, not SRGB8, to avoid ImGui brightness issues on main thread)
    GLboolean srgbWasEnabled = glIsEnabled(GL_FRAMEBUFFER_SRGB);
    if (srgbWasEnabled) {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }

    // Begin pass (binds framebuffer and sets viewport)
    Begin();

    // Setup command buffer with systems from context
    SetupCommandBuffer(context);

    // Clear the framebuffer
    RenderCommands::ClearData clearCmd{
        0.0f, 0.0f, 0.0f, 1.0f,  // Black background
        true,                      // clearColor
        false                      // no depth buffer
    };
    Submit(clearCmd);

    // Validate HDR texture is available
    if (context.hdrTextureID == 0) {
        spdlog::error("ToneMapRenderPass: Invalid HDR texture ID (0)!");
        End();
        return;
    }

    // Bind shader
    RenderCommands::BindShaderData bindShaderCmd{ m_ToneMappingShader };
    Submit(bindShaderCmd);

    // Bind HDR texture from context (set by HDRResolvePass)
    RenderCommands::BindTextureIDData bindTextureCmd{
        context.hdrTextureID,
        0,  // Texture unit 0
        m_ToneMappingShader,
        "u_HDRTexture"
    };
    Submit(bindTextureCmd);

    // Bind bloom texture if available (set by BloomRenderPass)
    if (m_BloomEnabled && context.frameData.bloomTexture != 0) {
        RenderCommands::BindTextureIDData bindBloomCmd{
            context.frameData.bloomTexture,
            1,  // Texture unit 1
            m_ToneMappingShader,
            "u_BloomTexture"
        };
        Submit(bindBloomCmd);

        RenderCommands::SetUniformFloatData bloomStrengthCmd{
            m_ToneMappingShader,
            "u_BloomStrength",
            m_BloomStrength
        };
        Submit(bloomStrengthCmd);

        RenderCommands::SetUniformIntData bloomEnabledCmd{
            m_ToneMappingShader,
            "u_BloomEnabled",
            1
        };
        Submit(bloomEnabledCmd);
    } else {
        // Disable bloom in shader
        RenderCommands::SetUniformIntData bloomEnabledCmd{
            m_ToneMappingShader,
            "u_BloomEnabled",
            0
        };
        Submit(bloomEnabledCmd);
    }

    // Set uniforms - READ FROM CONTEXT (framework pattern!)
    RenderCommands::SetUniformFloatData exposureCmd{
        m_ToneMappingShader,
        "u_Exposure",
        context.exposure
    };
    Submit(exposureCmd);

    RenderCommands::SetUniformFloatData avgLumCmd{
        m_ToneMappingShader,
        "u_AvgLuminance",
        context.avgLuminance
    };
    Submit(avgLumCmd);

    RenderCommands::SetUniformIntData methodCmd{
        m_ToneMappingShader,
        "u_Method",
        static_cast<int>(m_Method)
    };
    Submit(methodCmd);

    RenderCommands::SetUniformBoolData gammaCmd{
        m_ToneMappingShader,
        "u_EnableGamma",
        m_GammaCorrection
    };
    Submit(gammaCmd);

    // Render full-screen quad
    RenderCommands::DrawArraysData drawCmd{
        m_QuadVAO,
        6,             // 6 vertices (2 triangles)
        GL_TRIANGLES,
        0              // Start at vertex 0
    };
    Submit(drawCmd);

    // Execute all commands submitted to this pass's command buffer
    ExecuteCommands();

    // Store tone-mapped LDR result in frame data for subsequent passes
    context.frameData.postProcessBuffer = GetFramebuffer();

    // End pass
    End();

    // Restore GL_FRAMEBUFFER_SRGB state
    if (srgbWasEnabled) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    }
}

void ToneMapRenderPass::SetToneMappingShader(std::shared_ptr<Shader> shader)
{
    m_ToneMappingShader = shader;
}

void ToneMapRenderPass::CreateFullScreenQuad()
{
    // Full-screen quad in NDC coordinates
    // Two triangles covering the entire screen
    float quadVertices[] = {
        // Positions   // TexCoords
        -1.0f,  1.0f,  0.0f, 1.0f,  // Top-left
        -1.0f, -1.0f,  0.0f, 0.0f,  // Bottom-left
         1.0f, -1.0f,  1.0f, 0.0f,  // Bottom-right

        -1.0f,  1.0f,  0.0f, 1.0f,  // Top-left
         1.0f, -1.0f,  1.0f, 0.0f,  // Bottom-right
         1.0f,  1.0f,  1.0f, 1.0f   // Top-right
    };

    glGenVertexArrays(1, &m_QuadVAO);
    glGenBuffers(1, &m_QuadVBO);

    glBindVertexArray(m_QuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // TexCoord attribute (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                         (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

    spdlog::info("ToneMapRenderPass: Full-screen quad created");
}
