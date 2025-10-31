#include "Resources/OffsetTexture.h"
#include "Utility/PoissonDiskGenerator.h"
#include <spdlog/spdlog.h>
#include <cassert>

OffsetTexture::OffsetTexture(int windowSize, int filterSize)
    : m_WindowSize(windowSize), m_FilterSize(filterSize)
{
    assert(windowSize > 0 && "Window size must be positive");
    assert(filterSize > 0 && "Filter size must be positive");
    assert((filterSize % 2) == 0 && "Filter size must be even");

    // Generate Poisson disk offset data
    std::vector<float> data = PoissonDiskGenerator::GenerateOffsetTextureData(
        windowSize, filterSize
    );

    CreateTexture(data);

    spdlog::info("OffsetTexture: Created {}x{} tiling with {}x{} samples ({} total)",
                 windowSize, windowSize, filterSize, filterSize,
                 filterSize * filterSize);
}

OffsetTexture::~OffsetTexture() {
    if (m_TextureID != 0) {
        glDeleteTextures(1, &m_TextureID);
        m_TextureID = 0;
    }
}

void OffsetTexture::CreateTexture(const std::vector<float>& data) {
    int numFilterSamples = m_FilterSize * m_FilterSize;

    glGenTextures(1, &m_TextureID);
    glBindTexture(GL_TEXTURE_3D, m_TextureID);

    // Allocate 3D texture storage
    // Dimensions: (FilterSize²/2) × WindowSize × WindowSize
    // RGBA32F stores 2 offset vectors per texel (RG + BA)
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F,
                   numFilterSamples / 2, m_WindowSize, m_WindowSize);

    // Upload offset data
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0,
                    numFilterSamples / 2, m_WindowSize, m_WindowSize,
                    GL_RGBA, GL_FLOAT, data.data());

    // Use nearest filtering (we want exact offset values)
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Clamp to edge (avoid wrapping artifacts)
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_3D, 0);

    spdlog::debug("OffsetTexture: Created GL_TEXTURE_3D with ID {}", m_TextureID);
}

void OffsetTexture::Bind(uint32_t textureUnit) {
    assert(m_TextureID != 0 && "Texture must be created before binding");

    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_3D, m_TextureID);
}
