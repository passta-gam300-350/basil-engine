#include "Pipeline/ToneMapRenderPass.h"
#include "Pipeline/RenderContext.h"
#include <spdlog/spdlog.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>

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

    // Update framebuffer size to match current window before rendering
    UpdateFramebufferSize();

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

void ToneMapRenderPass::UpdateFramebufferSize()
{
    // Get current window size
    GLFWwindow* currentWindow = glfwGetCurrentContext();
    if (!currentWindow) return;

    int windowWidth, windowHeight;
    glfwGetFramebufferSize(currentWindow, &windowWidth, &windowHeight);

    // Check if we need to resize the framebuffer
    auto currentSpecs = m_Framebuffer->GetSpecification();
    if (currentSpecs.Width != static_cast<uint32_t>(windowWidth) ||
        currentSpecs.Height != static_cast<uint32_t>(windowHeight)) {

        // Resize the framebuffer to match window size
        m_Framebuffer->Resize(static_cast<uint32_t>(windowWidth), static_cast<uint32_t>(windowHeight));

        // Update the viewport to match the new framebuffer size
        SetViewport(Viewport(0, 0, static_cast<uint32_t>(windowWidth), static_cast<uint32_t>(windowHeight)));
    }
}
