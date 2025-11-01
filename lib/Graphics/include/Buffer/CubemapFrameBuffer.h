#pragma once

#include <cstdint>
#include <glad/glad.h>

/**
 * CubemapFrameBuffer - Framebuffer with cubemap depth attachment
 *
 * Similar to FrameBuffer but creates a cubemap texture instead of 2D texture.
 * Used for omnidirectional shadow mapping (point light shadows).
 *
 * The geometry shader renders to all 6 faces in one pass using gl_Layer.
 */
class CubemapFrameBuffer
{
public:
    CubemapFrameBuffer(uint32_t cubemapSize);
    ~CubemapFrameBuffer();

    // No copy/move (manages OpenGL resources)
    CubemapFrameBuffer(const CubemapFrameBuffer&) = delete;
    CubemapFrameBuffer& operator=(const CubemapFrameBuffer&) = delete;

    void Bind() const;
    void Unbind() const;

    // Getters
    uint32_t GetFramebufferID() const { return m_FBOHandle; }
    uint32_t GetCubemapTextureID() const { return m_CubemapTexture; }
    uint32_t GetSize() const { return m_CubemapSize; }

private:
    void CreateCubemap();
    void CreateFramebuffer();

    uint32_t m_FBOHandle = 0;
    uint32_t m_CubemapTexture = 0;
    uint32_t m_CubemapSize = 0;
};
