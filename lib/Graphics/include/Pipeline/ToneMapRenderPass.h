#pragma once
#include "RenderPass.h"
#include "../Resources/Shader.h"
#include <memory>

/**
 * @brief Tone Mapping Render Pass
 *
 * Converts HDR (high dynamic range) to LDR (low dynamic range) for display.
 * Supports multiple tone mapping operators:
 * - None: Pass-through (no tone mapping)
 * - Reinhard: Simple operator (hdr / (hdr + 1))
 * - ACES: Filmic operator (cinematic look)
 * - Exposure: Exposure-based Reinhard with auto-exposure
 *
 * Also applies gamma correction (linear � sRGB).
 */
class ToneMapRenderPass : public RenderPass {
public:
    enum class Method {
        None = 0,
        Reinhard = 1,
        ACES = 2,
        Exposure = 3  // Default (ogldev's method)
    };

    ToneMapRenderPass();
    ~ToneMapRenderPass() override = default;

    void Execute(RenderContext& context) override;

    // Configuration
    void SetToneMappingShader(std::shared_ptr<Shader> shader);
    void SetMethod(Method method) { m_Method = method; }
    void EnableGammaCorrection(bool enable) { m_GammaCorrection = enable; }
    void SetBloomStrength(float strength) { m_BloomStrength = strength; }
    void EnableBloom(bool enable) { m_BloomEnabled = enable; }

    // Getters
    Method GetMethod() const { return m_Method; }
    bool IsGammaCorrectionEnabled() const { return m_GammaCorrection; }
    float GetBloomStrength() const { return m_BloomStrength; }
    bool IsBloomEnabled() const { return m_BloomEnabled; }

private:
    std::shared_ptr<Shader> m_ToneMappingShader;

    // Configuration (exposure/luminance come from context!)
    Method m_Method = Method::Exposure;
    bool m_GammaCorrection = true;

    // Bloom configuration
    float m_BloomStrength = 0.04f;
    bool m_BloomEnabled = true;

    // Full-screen quad
    uint32_t m_QuadVAO = 0;
    uint32_t m_QuadVBO = 0;

    void CreateFullScreenQuad();
    void UpdateFramebufferSize();  // Resize framebuffer to match window
};
