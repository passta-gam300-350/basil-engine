#include "Pipeline/HDRLuminancePass.h"
#include "Pipeline/RenderContext.h"
#include <spdlog/spdlog.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cmath>

HDRLuminancePass::HDRLuminancePass()
    : RenderPass("HDRLuminancePass")
{
}

void HDRLuminancePass::Execute(RenderContext& context)
{
    if (!m_ComputeShader) {
        spdlog::warn("HDRLuminancePass: No compute shader set!");
        return;
    }

    // Initialize buffer if not done yet
    if (!m_LuminanceBuffer || m_NumGroupsX == 0 || m_NumGroupsY == 0) {
        InitializeBuffer(context.frameData.viewportWidth, context.frameData.viewportHeight);
    }

    // Bind HDR texture from context (set by HDRResolvePass)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, context.hdrTextureID);

    // Activate compute shader
    m_ComputeShader->use();
    m_ComputeShader->setInt("u_HdrTexture", 0);

    // Bind SSBO to binding point 1
    m_LuminanceBuffer->BindBase(1);

    // Dispatch compute shader
    // Each work group = 10x10 threads
    glDispatchCompute(m_NumGroupsX, m_NumGroupsY, 1);

    // Memory barrier: ensure compute shader writes complete before CPU reads
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Calculate exposure and WRITE TO CONTEXT (framework pattern!)
    CalculateExposure(context);

    // Unbind SSBO
    m_LuminanceBuffer->Unbind();
}

void HDRLuminancePass::SetComputeShader(std::shared_ptr<Shader> shader)
{
    m_ComputeShader = shader;
}

void HDRLuminancePass::CalculateExposure(RenderContext& context)
{
    // Map SSBO for CPU reading (GPU to CPU transfer, may stall pipeline)
    float* pLuminance = static_cast<float*>(m_LuminanceBuffer->Map(GL_READ_ONLY));

    if (!pLuminance) {
        spdlog::error("HDRLuminancePass: Failed to map SSBO!");
        return;
    }

    // Sum all tile luminances
    float sum = 0.0f;
    uint32_t numTiles = m_NumGroupsX * m_NumGroupsY;

    for (uint32_t i = 0; i < numTiles; ++i) {
        sum += pLuminance[i];
    }

    // Unmap SSBO
    m_LuminanceBuffer->Unmap();

    // Calculate geometric mean: exp(average of log luminances)
    uint32_t totalPixels = m_NumGroupsX * m_NumGroupsY * m_LocalSizeX * m_LocalSizeY;
    float avgLuminance = std::exp(sum / static_cast<float>(totalPixels));

    // Calculate exposure using photographic middle gray (0.18)
    // Formula from ogldev: Exposure = targetGray / AvgLuminance
    const float targetGray = 0.18f;
    float exposure = targetGray / avgLuminance;

    // Clamp exposure to reasonable range (prevent extreme values)
    exposure = glm::clamp(exposure, 0.1f, 10.0f);

    // WRITE TO CONTEXT (framework pattern!)
    context.avgLuminance = avgLuminance;
    context.exposure = exposure;

    // Debug logging (uncomment if needed)
    // spdlog::debug("HDRLuminancePass: AvgLum={:.4f}, Exposure={:.4f}",
    //               context.avgLuminance, context.exposure);
}

uint32_t HDRLuminancePass::AlignUpToMultiple(uint32_t value, uint32_t multiple)
{
    return ((value + multiple - 1) / multiple) * multiple;
}

void HDRLuminancePass::InitializeBuffer(uint32_t width, uint32_t height)
{
    // Calculate number of work groups needed to cover screen
    // Each work group processes 10�10 pixels
    m_NumGroupsX = AlignUpToMultiple(width, m_LocalSizeX) / m_LocalSizeX;
    m_NumGroupsY = AlignUpToMultiple(height, m_LocalSizeY) / m_LocalSizeY;

    uint32_t numTiles = m_NumGroupsX * m_NumGroupsY;

    // Create SSBO to store per-tile results (one float per tile)
    m_LuminanceBuffer = std::make_unique<ShaderStorageBuffer>(nullptr, numTiles * sizeof(float), GL_DYNAMIC_DRAW);

    spdlog::info("HDRLuminancePass: Initialized ({}x{}, {} tiles, {}x{} groups)",
                 width, height, numTiles, m_NumGroupsX, m_NumGroupsY);
}
