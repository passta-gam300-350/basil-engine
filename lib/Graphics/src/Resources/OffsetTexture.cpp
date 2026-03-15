/******************************************************************************/
/*!
\file   OffsetTexture.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Offset texture implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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

    // DSA: Create 3D texture without binding
    glCreateTextures(GL_TEXTURE_3D, 1, &m_TextureID);

    // DSA: Allocate 3D texture immutable storage
    // Dimensions: (FilterSize²/2) × WindowSize × WindowSize
    // RGBA32F stores 2 offset vectors per texel (RG + BA)
    glTextureStorage3D(m_TextureID, 1, GL_RGBA32F,
                       numFilterSamples / 2, m_WindowSize, m_WindowSize);

    // DSA: Upload offset data
    glTextureSubImage3D(m_TextureID, 0, 0, 0, 0,
                        numFilterSamples / 2, m_WindowSize, m_WindowSize,
                        GL_RGBA, GL_FLOAT, data.data());

    // DSA: Use nearest filtering (we want exact offset values)
    glTextureParameteri(m_TextureID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_TextureID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // DSA: Clamp to edge (avoid wrapping artifacts)
    glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    spdlog::debug("OffsetTexture: Created GL_TEXTURE_3D with ID {}", m_TextureID);
}

void OffsetTexture::Bind(uint32_t textureUnit) {
    assert(m_TextureID != 0 && "Texture must be created before binding");

    // DSA: Bind 3D texture directly to specified texture unit
    glBindTextureUnit(textureUnit, m_TextureID);
}
