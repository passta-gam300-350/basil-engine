#include "Pipeline/HDRLuminancePass.h"
#include "Pipeline/RenderContext.h"
#include <spdlog/spdlog.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cmath>
#include <cstring>  // For std::memset

HDRLuminancePass::HDRLuminancePass()
    : RenderPass("HDRLuminancePass")
{
}

void HDRLuminancePass::Execute(RenderContext& context)
{
    if (!m_ComputeShader) {
        // No compute shader - use fixed safe values instead of NaN!
        context.avgLuminance = 0.5f;   // Middle gray
        context.exposure = 1.0f;        // Neutral exposure

        static bool warned = false;
        if (!warned) {
            spdlog::warn("HDRLuminancePass: No compute shader set! Using fixed exposure=1.0");
            warned = true;
        }
        return;
    }

    // Initialize or resize buffer if viewport size changed
    uint32_t currentWidth = context.frameData.viewportWidth;
    uint32_t currentHeight = context.frameData.viewportHeight;

    if (!m_LuminanceBuffer || m_LastWidth != currentWidth || m_LastHeight != currentHeight) {
        InitializeBuffer(currentWidth, currentHeight);
        m_LastWidth = currentWidth;
        m_LastHeight = currentHeight;
    }

    // Begin the pass (no framebuffer needed for compute)
    Begin();

    // Setup command buffer with systems from context
    SetupCommandBuffer(context);

    // Bind HDR texture from context (set by HDRResolvePass)
    RenderCommands::BindTextureIDData bindTextureCmd{
        context.hdrTextureID,
        0,  // Texture unit 0
        m_ComputeShader,
        "u_HdrTexture"
    };
    Submit(bindTextureCmd);

    // Bind SSBO to binding point 1
    RenderCommands::BindSSBOData bindSSBOCmd{
        m_LuminanceBuffer->GetSSBOHandle(),
        1  // Binding point 1
    };
    Submit(bindSSBOCmd);

    // Dispatch compute shader (each work group = 10x10 threads)
    RenderCommands::DispatchComputeData dispatchCmd{
        m_ComputeShader,
        m_NumGroupsX,
        m_NumGroupsY,
        1  // Z dimension
    };
    Submit(dispatchCmd);

    // Memory barrier: ensure compute shader writes complete before CPU reads
    RenderCommands::MemoryBarrierData barrierCmd{
        GL_SHADER_STORAGE_BARRIER_BIT
    };
    Submit(barrierCmd);

    // Execute all commands submitted to this pass's command buffer
    ExecuteCommands();

    // Calculate exposure and WRITE TO CONTEXT (framework pattern!)
    // This happens after ExecuteCommands() to ensure compute shader completed
    CalculateExposure(context);

    // Note: SSBO remains bound and persistently mapped - no need to unbind

    // End the pass
    End();
}

void HDRLuminancePass::SetComputeShader(std::shared_ptr<Shader> shader)
{
    m_ComputeShader = shader;
}

void HDRLuminancePass::CalculateExposure(RenderContext& context)
{
    // Access persistent pointer (no stall - buffer is always mapped)
    float* pLuminance = static_cast<float*>(m_LuminanceBuffer->GetPersistentPtr());

    if (!pLuminance) {
        spdlog::error("HDRLuminancePass: Failed to access persistent buffer!");
        return;
    }

    // Sum all tile luminances
    float sum = 0.0f;
    uint32_t numTiles = m_NumGroupsX * m_NumGroupsY;
    uint32_t invalidCount = 0;

    for (uint32_t i = 0; i < numTiles; ++i) {
        float val = pLuminance[i];
        // Skip NaN or infinite values from compute shader
        if (std::isfinite(val)) {
            sum += val;
        } else {
            invalidCount++;
        }
    }

    // Log if compute shader produced invalid values
    if (invalidCount > 0) {
        spdlog::warn("HDRLuminancePass: Compute shader produced {} invalid/NaN values out of {} tiles!",
                     invalidCount, numTiles);
    }

    // No need to unmap - buffer is persistently mapped!

    // Calculate geometric mean: exp(average of log luminances)
    uint32_t totalPixels = m_NumGroupsX * m_NumGroupsY * m_LocalSizeX * m_LocalSizeY;
    float avgLogLum = sum / static_cast<float>(totalPixels);
    float avgLuminance = std::exp(avgLogLum);

    // Validate avgLuminance - if NaN or invalid, use safe default
    if (!std::isfinite(avgLuminance) || avgLuminance <= 0.0f) {
        spdlog::warn("HDRLuminancePass: Invalid avgLuminance ({:.3f}) from sum={:.3f}, avgLogLum={:.3f}, totalPixels={}. Using default 0.5",
                     avgLuminance, sum, avgLogLum, totalPixels);
        avgLuminance = 0.5f;
    }

    // DYNAMIC EXPOSURE: Calculate exposure based on scene brightness
    // This auto-adapts to different lighting conditions
    const float targetGray = 0.18f;  // Standard photographic middle gray
    float exposure = targetGray / glm::max(avgLuminance, 0.001f);  // Avoid division by zero

    // Clamp exposure to reasonable range (lowered max from 3.0 to 2.0 to reduce over-brightening)
    exposure = glm::clamp(exposure, 0.1f, 1.0f);

    // Validate exposure - if still NaN, use safe default
    if (!std::isfinite(exposure)) {
        spdlog::warn("HDRLuminancePass: Invalid exposure ({:.3f}), using default 1.0", exposure);
        exposure = 1.0f;
    }

    // Alternative (fixed exposure - use this to match ogldev exactly):
    // const float exposure = 0.4457f;  // Fixed value matching ogldev

    // WRITE TO CONTEXT (framework pattern!)
    context.avgLuminance = avgLuminance;
    context.exposure = exposure;

    // Debug logging - enabled to diagnose brightness issue
    spdlog::info("HDRLuminancePass: AvgLum={:.4f}, Exposure={:.4f}",
                  context.avgLuminance, context.exposure);
}

uint32_t HDRLuminancePass::AlignUpToMultiple(uint32_t value, uint32_t multiple)
{
    return ((value + multiple - 1) / multiple) * multiple;
}

void HDRLuminancePass::InitializeBuffer(uint32_t width, uint32_t height)
{
    // Calculate number of work groups needed to cover screen
    // Each work group processes 10x10 pixels
    m_NumGroupsX = AlignUpToMultiple(width, m_LocalSizeX) / m_LocalSizeX;
    m_NumGroupsY = AlignUpToMultiple(height, m_LocalSizeY) / m_LocalSizeY;

    uint32_t numTiles = m_NumGroupsX * m_NumGroupsY;
    uint32_t bufferSize = numTiles * sizeof(float);

    // Create SSBO with persistent mapping (GL 4.4+)
    // Flags: GL_MAP_READ_BIT - allow CPU reads
    //        GL_MAP_PERSISTENT_BIT - keep mapped across draw calls
    //        GL_MAP_COHERENT_BIT - automatic synchronization (no manual flush needed)
    m_LuminanceBuffer = std::make_unique<ShaderStorageBuffer>();
    m_LuminanceBuffer->CreatePersistentBuffer(
        bufferSize,
        GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT
    );

    // Zero-initialize the buffer to prevent garbage data on first frame
    float* pLuminance = static_cast<float*>(m_LuminanceBuffer->GetPersistentPtr());
    if (pLuminance) {
        std::memset(pLuminance, 0, bufferSize);
    }

    spdlog::info("HDRLuminancePass: Initialized with persistent buffer ({}x{}, {} tiles, {}x{} groups)",
                 width, height, numTiles, m_NumGroupsX, m_NumGroupsY);
}
