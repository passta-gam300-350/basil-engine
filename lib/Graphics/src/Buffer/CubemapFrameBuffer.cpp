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
    // Generate cubemap texture
    glGenTextures(1, &m_CubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_CubemapTexture);

    // Create 6 faces of depth cubemap
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                    m_CubemapSize, m_CubemapSize, 0,
                    GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }

    // Set texture parameters for depth cubemap
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void CubemapFrameBuffer::CreateFramebuffer()
{
    // Generate framebuffer
    glGenFramebuffers(1, &m_FBOHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBOHandle);

    // Attach cubemap as depth attachment (geometry shader will render to all 6 faces)
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_CubemapTexture, 0);

    // Disable color buffer (depth-only rendering)
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
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

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
