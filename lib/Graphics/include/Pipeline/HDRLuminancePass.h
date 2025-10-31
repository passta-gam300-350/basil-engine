#pragma once
#include "RenderPass.h"
#include "../Resources/Shader.h"
#include "../Buffer/ShaderStorageBuffer.h"
#include <memory>

/**
 * @brief HDR Luminance Pass - Auto-Exposure Calculation
 *
 * Uses compute shader to calculate average scene luminance in parallel.
 * Implements ogldev's auto-exposure algorithm:
 * 1. Divide screen into 10x10 pixel tiles
 * 2. Each tile calculates sum of log luminances
 * 3. CPU reads back and computes geometric mean
 * 4. Calculate exposure: Exposure = 0.18 / AvgLuminance
 */
class HDRLuminancePass : public RenderPass {
public:
    HDRLuminancePass();
    ~HDRLuminancePass() override = default;

    void Execute(RenderContext& context) override;

    // Configuration
    void SetComputeShader(std::shared_ptr<Shader> shader);

private:
    std::shared_ptr<Shader> m_ComputeShader;
    std::unique_ptr<ShaderStorageBuffer> m_LuminanceBuffer;

    // Compute dispatch parameters
    uint32_t m_NumGroupsX = 0;
    uint32_t m_NumGroupsY = 0;
    uint32_t m_LocalSizeX = 10;  // Must match compute shader
    uint32_t m_LocalSizeY = 10;

    // Track viewport size for resize detection
    uint32_t m_LastWidth = 0;
    uint32_t m_LastHeight = 0;

    // Temporal smoothing for exposure adaptation
    float m_PreviousExposure = 1.0f;           // Last frame's exposure value
    float m_ExposureAdaptationSpeed = 0.05f;   // Blend factor (0.05 = 5% per frame, ~20 frames to adapt)

    // Custom resize handling (override from RenderPass)
    void OnResize(uint32_t newWidth, uint32_t newHeight) override;

    void CalculateExposure(RenderContext& context);
    uint32_t AlignUpToMultiple(uint32_t value, uint32_t multiple);
    void InitializeBuffer(uint32_t width, uint32_t height);
};
