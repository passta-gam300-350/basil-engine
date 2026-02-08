/******************************************************************************/
/*!
\file   OffsetTexture.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Offset texture for shadow sampling

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <glad/glad.h>
#include <vector>
#include <cstdint>

/**
 * 3D Texture containing Poisson disk offset patterns for random shadow sampling
 *
 * Texture dimensions: (FilterSize^2/2) × WindowSize × WindowSize
 * - Depth (x): Stores offset pairs (RG + BA per texel = 2 offset vectors)
 * - Height/Width (y,z): Screen-space tiling pattern
 */
class OffsetTexture {
public:
    /**
     * @param windowSize Size of tiling pattern (4-16 recommended)
     * @param filterSize Samples per dimension (4 = 16 total samples)
     */
    OffsetTexture(int windowSize, int filterSize);
    ~OffsetTexture();

    // Prevent copying
    OffsetTexture(const OffsetTexture&) = delete;
    OffsetTexture& operator=(const OffsetTexture&) = delete;

    void Bind(uint32_t textureUnit);
    uint32_t GetTextureID() const { return m_TextureID; }
    int GetWindowSize() const { return m_WindowSize; }
    int GetFilterSize() const { return m_FilterSize; }

private:
    uint32_t m_TextureID = 0;
    int m_WindowSize;
    int m_FilterSize;

    void CreateTexture(const std::vector<float>& data);
};
