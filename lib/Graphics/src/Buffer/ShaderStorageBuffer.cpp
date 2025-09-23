#include "Buffer/ShaderStorageBuffer.h"
#include <iostream>
#include <cassert>

ShaderStorageBuffer::ShaderStorageBuffer(const void* data, uint32_t size, GLenum usage)
    : m_Size(size), m_Usage(usage), m_IsMapped(false)
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
        glDeleteBuffers(1, &m_SSBOHandle);
    }
}

ShaderStorageBuffer::ShaderStorageBuffer(ShaderStorageBuffer&& other) noexcept
    : m_SSBOHandle(other.m_SSBOHandle), m_Size(other.m_Size), 
      m_Usage(other.m_Usage), m_IsMapped(other.m_IsMapped)
{
    other.m_SSBOHandle = 0;
    other.m_Size = 0;
    other.m_IsMapped = false;
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
        
        // Reset other
        other.m_SSBOHandle = 0;
        other.m_Size = 0;
        other.m_IsMapped = false;
    }
    return *this;
}

void ShaderStorageBuffer::BindBase(uint32_t bindingPoint) const
{
    assert(!m_IsMapped && "Cannot bind SSBO while mapped");
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_SSBOHandle);
}

void ShaderStorageBuffer::Bind() const
{
    assert(!m_IsMapped && "Cannot bind SSBO while mapped");
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
    
    if (newSize == m_Size) return;
    
    m_Size = newSize;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBOHandle);
    glBufferData(GL_SHADER_STORAGE_BUFFER, newSize, nullptr, m_Usage);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void* ShaderStorageBuffer::Map(GLenum access)
{
    assert(!m_IsMapped && "Buffer already mapped");
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBOHandle);
    void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, access);
    
    if (ptr) {
        m_IsMapped = true;
    } else {
        std::cerr << "Failed to map SSBO" << std::endl;
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
        std::cerr << "Failed to unmap SSBO - data corruption possible" << std::endl;
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