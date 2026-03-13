/******************************************************************************/
/*!
\file   CubemapFrameBuffer.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Cubemap framebuffer implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Buffer/CubemapFrameBuffer.h"
#include <spdlog/spdlog.h>
#include <glad/glad.h>

CubemapFrameBuffer::CubemapFrameBuffer(uint32_t cubemapSize)
    : m_CubemapSize(cubemapSize)
{
    if (m_CubemapSize == 0) {
        spdlog::warn("CubemapFrameBuffer: Invalid size (0), using default 1024");
        m_CubemapSize = 1024;
    }

    CreateCubemap();
    CreateFramebuffer();

    spdlog::debug("CubemapFrameBuffer created successfully (size: {}x{})",
                 m_CubemapSize, m_CubemapSize);
}

CubemapFrameBuffer::~CubemapFrameBuffer()
{
    if (m_FBOHandle != 0) {
        glDeleteFramebuffers(1, &m_FBOHandle);
    }

    if (m_CubemapTexture != 0) {
        glDeleteTextures(1, &m_CubemapTexture);
    }
}

void CubemapFrameBuffer::CreateCubemap()
{
    // DSA: Create cubemap texture without binding
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_CubemapTexture);

    // DSA: Allocate immutable storage for all 6 faces
    glTextureStorage2D(m_CubemapTexture, 1, GL_DEPTH_COMPONENT32F, m_CubemapSize, m_CubemapSize);

    // DSA: Set texture parameters for depth cubemap without binding
    glTextureParameteri(m_CubemapTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_CubemapTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_CubemapTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_CubemapTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_CubemapTexture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // DSA: No need to unbind - we never bound it!
}

void CubemapFrameBuffer::CreateFramebuffer()
{
    // DSA: Create framebuffer without binding
    glCreateFramebuffers(1, &m_FBOHandle);

    // DSA: Attach cubemap as depth attachment without binding (geometry shader will render to all 6 faces)
    glNamedFramebufferTexture(m_FBOHandle, GL_DEPTH_ATTACHMENT, m_CubemapTexture, 0);

    // DSA: Disable color buffer (depth-only rendering) without binding
    glNamedFramebufferDrawBuffer(m_FBOHandle, GL_NONE);
    glNamedFramebufferReadBuffer(m_FBOHandle, GL_NONE);

    // DSA: Check framebuffer completeness without binding
    GLenum status = glCheckNamedFramebufferStatus(m_FBOHandle, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::error("CubemapFrameBuffer: Framebuffer incomplete! Status: 0x{:x}", status);

        switch (status) {
        case GL_FRAMEBUFFER_UNDEFINED:
            spdlog::error("  -> GL_FRAMEBUFFER_UNDEFINED");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            spdlog::error("  -> GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            spdlog::error("  -> GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            spdlog::error("  -> GL_FRAMEBUFFER_UNSUPPORTED");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            spdlog::error("  -> GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS");
            break;
        default:
            spdlog::error("  -> Unknown error");
            break;
        }
    }

    // DSA: No need to unbind - we never bound it!
}

void CubemapFrameBuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBOHandle);
    glViewport(0, 0, m_CubemapSize, m_CubemapSize);
}

void CubemapFrameBuffer::Unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
