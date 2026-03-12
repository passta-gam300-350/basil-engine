/******************************************************************************/
/*!
\file   ShaderStorageBuffer.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Shader storage buffer implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Buffer/ShaderStorageBuffer.h"
#include <spdlog/spdlog.h>
#include <cassert>

ShaderStorageBuffer::ShaderStorageBuffer(const void* data, uint32_t size, GLenum usage)
    : m_Size(size), m_Usage(usage), m_IsMapped(false),
      m_IsPersistent(false), m_PersistentPtr(nullptr), m_PersistentFlags(0)
{
    // DSA: Create and initialize buffer in one step, no binding required
    glCreateBuffers(1, &m_SSBOHandle);

    if (size > 0) {
        glNamedBufferData(m_SSBOHandle, size, data, usage);
    }
}

ShaderStorageBuffer::~ShaderStorageBuffer()
{
    if (m_SSBOHandle != 0) {
        // Persistent buffers are automatically unmapped when deleted
        // No need to manually unmap
        glDeleteBuffers(1, &m_SSBOHandle);
    }
}

ShaderStorageBuffer::ShaderStorageBuffer(ShaderStorageBuffer&& other) noexcept
    : m_SSBOHandle(other.m_SSBOHandle), m_Size(other.m_Size),
      m_Usage(other.m_Usage), m_IsMapped(other.m_IsMapped),
      m_IsPersistent(other.m_IsPersistent), m_PersistentPtr(other.m_PersistentPtr),
      m_PersistentFlags(other.m_PersistentFlags)
{
    other.m_SSBOHandle = 0;
    other.m_Size = 0;
    other.m_IsMapped = false;
    other.m_IsPersistent = false;
    other.m_PersistentPtr = nullptr;
    other.m_PersistentFlags = 0;
}

ShaderStorageBuffer& ShaderStorageBuffer::operator=(ShaderStorageBuffer&& other) noexcept
{
    if (this != &other) {
        // Clean up existing resource
        if (m_SSBOHandle != 0) {
            glDeleteBuffers(1, &m_SSBOHandle);
        }

        // Move from other
        m_SSBOHandle = other.m_SSBOHandle;
        m_Size = other.m_Size;
        m_Usage = other.m_Usage;
        m_IsMapped = other.m_IsMapped;
        m_IsPersistent = other.m_IsPersistent;
        m_PersistentPtr = other.m_PersistentPtr;
        m_PersistentFlags = other.m_PersistentFlags;

        // Reset other
        other.m_SSBOHandle = 0;
        other.m_Size = 0;
        other.m_IsMapped = false;
        other.m_IsPersistent = false;
        other.m_PersistentPtr = nullptr;
        other.m_PersistentFlags = 0;
    }
    return *this;
}

void ShaderStorageBuffer::BindBase(uint32_t bindingPoint) const
{
    // Persistent buffers can be bound while mapped
    assert((!m_IsMapped || m_IsPersistent) && "Cannot bind SSBO while mapped (unless persistent)");
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_SSBOHandle);
}

void ShaderStorageBuffer::Bind() const
{
    // Persistent buffers can be bound while mapped
    assert((!m_IsMapped || m_IsPersistent) && "Cannot bind SSBO while mapped (unless persistent)");
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBOHandle);
}

void ShaderStorageBuffer::Unbind() const
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
{
    assert(!m_IsMapped && "Cannot set data while buffer is mapped");
    assert(offset + size <= m_Size && "Data exceeds buffer size");

    // DSA: Update buffer data without binding
    glNamedBufferSubData(m_SSBOHandle, offset, size, data);
}

void ShaderStorageBuffer::Resize(uint32_t newSize)
{
    assert(!m_IsMapped && "Cannot resize while buffer is mapped");
    assert(!m_IsPersistent && "Cannot resize persistent buffers - they are immutable");

    if (newSize == m_Size) return;

    m_Size = newSize;
    // DSA: Resize buffer without binding
    glNamedBufferData(m_SSBOHandle, newSize, nullptr, m_Usage);
}

void ShaderStorageBuffer::CreatePersistentBuffer(uint32_t size, GLbitfield flags)
{
    assert(!m_IsPersistent && "Buffer is already persistent");
    assert(size > 0 && "Buffer size must be positive");

    // Store flags and size
    m_Size = size;
    m_PersistentFlags = flags;

    // DSA: Create immutable storage with glNamedBufferStorage (GL 4.5+)
    glNamedBufferStorage(m_SSBOHandle, size, nullptr, flags);

    // DSA: Map the buffer persistently without binding
    m_PersistentPtr = glMapNamedBufferRange(m_SSBOHandle, 0, size, flags);

    if (!m_PersistentPtr) {
        spdlog::error("Failed to create persistent mapped buffer!");
        return;
    }

    m_IsPersistent = true;

    spdlog::info("Created persistent mapped SSBO (size: {} bytes)", size);
}

void* ShaderStorageBuffer::Map(GLenum access)
{
    assert(!m_IsMapped && "Buffer already mapped");

    // DSA: Map buffer without binding
    void* ptr = glMapNamedBuffer(m_SSBOHandle, access);

    if (ptr) {
        m_IsMapped = true;
    } else {
        spdlog::error("Failed to map SSBO");
    }

    return ptr;
}

void ShaderStorageBuffer::Unmap()
{
    assert(m_IsMapped && "Buffer not mapped");

    // DSA: Unmap buffer without binding
    if (glUnmapNamedBuffer(m_SSBOHandle)) {
        m_IsMapped = false;
    } else {
        spdlog::error("Failed to unmap SSBO - data corruption possible");
    }
}

void ShaderStorageBuffer::GetData(void* data, uint32_t size, uint32_t offset) const
{
    assert(!m_IsMapped && "Cannot get data while buffer is mapped");
    assert(offset + size <= m_Size && "Read exceeds buffer size");

    // DSA: Read buffer data without binding
    glGetNamedBufferSubData(m_SSBOHandle, offset, size, data);
}

void ShaderStorageBuffer::MemoryBarrier(GLbitfield barriers)
{
    glMemoryBarrier(barriers);
}