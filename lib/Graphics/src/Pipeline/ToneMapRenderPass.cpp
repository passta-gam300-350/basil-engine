#include "Pipeline/ToneMapRenderPass.h"
#include "Pipeline/RenderContext.h"
#include <spdlog/spdlog.h>
#include <glad/glad.h>

ToneMapRenderPass::ToneMapRenderPass()
    : RenderPass("ToneMapPass", FBOSpecs{
        1280, 720,
        {
            { FBOTextureFormat::RGB8 },  // LDR output (8-bit RGB)
        },
        1  // No MSAA for post-process
    })
{
    CreateFullScreenQuad();
}

void ToneMapRenderPass::Execute(RenderContext& context)
{
    if (!m_ToneMappingShader) {
        spdlog::warn("ToneMapRenderPass: No tone mapping shader set!");
        return;
    }

    // Begin pass (binds framebuffer and sets viewport)
    Begin();

    // Clear the framebuffer
    RenderCommands::ClearData clearCmd{
        0.0f, 0.0f, 0.0f, 1.0f,  // Black background
        true,                      // clearColor
        false                      // no depth buffer
    };
    Submit(clearCmd);

    // Bind HDR texture from context (set by HDRResolvePass)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, context.hdrTextureID);

    // Activate tone mapping shader
    m_ToneMappingShader->use();

    // Set uniforms - READ FROM CONTEXT (framework pattern!)
    m_ToneMappingShader->setInt("u_HDRTexture", 0);
    m_ToneMappingShader->setFloat("u_Exposure", context.exposure);
    m_ToneMappingShader->setFloat("u_AvgLuminance", context.avgLuminance);
    m_ToneMappingShader->setInt("u_Method", static_cast<int>(m_Method));
    m_ToneMappingShader->setBool("u_EnableGamma", m_GammaCorrection);

    // Render full-screen quad
    glBindVertexArray(m_QuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // Store tone-mapped LDR result in frame data for subsequent passes
    context.frameData.postProcessBuffer = GetFramebuffer();

    // End pass
    End();
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
