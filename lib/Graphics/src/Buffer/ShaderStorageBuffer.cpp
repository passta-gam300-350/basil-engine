#include "Buffer/ShaderStorageBuffer.h"
#include <spdlog/spdlog.h>
#include <cassert>

ShaderStorageBuffer::ShaderStorageBuffer(const void* data, uint32_t size, GLenum usage)
    : m_Size(size), m_Usage(usage), m_IsMapped(false),
      m_IsPersistent(false), m_PersistentPtr(nullptr), m_PersistentFlags(0)
{
    glGenBuffers(1, &m_SSBOHandle);

    if (size > 0) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBOHandle);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, usage);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBOHandle);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::Resize(uint32_t newSize)
{
    assert(!m_IsMapped && "Cannot resize while buffer is mapped");
    assert(!m_IsPersistent && "Cannot resize persistent buffers - they are immutable");

    if (newSize == m_Size) return;

    m_Size = newSize;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBOHandle);
    glBufferData(GL_SHADER_STORAGE_BUFFER, newSize, nullptr, m_Usage);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::CreatePersistentBuffer(uint32_t size, GLbitfield flags)
{
    assert(!m_IsPersistent && "Buffer is already persistent");
    assert(size > 0 && "Buffer size must be positive");

    // Store flags and size
    m_Size = size;
    m_PersistentFlags = flags;

    // Create immutable storage with glBufferStorage (GL 4.4+)
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBOHandle);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, nullptr, flags);

    // Map the buffer persistently
    m_PersistentPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, size, flags);

    if (!m_PersistentPtr) {
        spdlog::error("Failed to create persistent mapped buffer!");
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return;
    }

    m_IsPersistent = true;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    spdlog::info("Created persistent mapped SSBO (size: {} bytes)", size);
}

void* ShaderStorageBuffer::Map(GLenum access)
{
    assert(!m_IsMapped && "Buffer already mapped");
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBOHandle);
    void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, access);
    
    if (ptr) {
        m_IsMapped = true;
    } else {
        spdlog::error("Failed to map SSBO");
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
    
    return ptr;
}

void ShaderStorageBuffer::Unmap()
{
    assert(m_IsMapped && "Buffer not mapped");
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBOHandle);
    if (glUnmapBuffer(GL_SHADER_STORAGE_BUFFER)) {
        m_IsMapped = false;
    } else {
        spdlog::error("Failed to unmap SSBO - data corruption possible");
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::GetData(void* data, uint32_t size, uint32_t offset) const
{
    assert(!m_IsMapped && "Cannot get data while buffer is mapped");
    assert(offset + size <= m_Size && "Read exceeds buffer size");
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBOHandle);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::MemoryBarrier(GLbitfield barriers)
{
    glMemoryBarrier(barriers);
}