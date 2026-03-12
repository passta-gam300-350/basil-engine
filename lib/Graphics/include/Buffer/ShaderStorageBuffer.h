/******************************************************************************/
/*!
\file   ShaderStorageBuffer.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the ShaderStorageBuffer class for OpenGL SSBO management with typed wrapper

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <cstdint>
#include <vector>
#include <glad/glad.h>

class ShaderStorageBuffer
{
public:
    // Constructor with initial data (optional)
    ShaderStorageBuffer(const void* data = nullptr, uint32_t size = 0, GLenum usage = GL_DYNAMIC_DRAW);
    ~ShaderStorageBuffer();

    // Non-copyable but movable
    ShaderStorageBuffer(const ShaderStorageBuffer&) = delete;
    ShaderStorageBuffer& operator=(const ShaderStorageBuffer&) = delete;
    ShaderStorageBuffer(ShaderStorageBuffer&& other) noexcept;
    ShaderStorageBuffer& operator=(ShaderStorageBuffer&& other) noexcept;

    // Create persistent mapped buffer (GL 4.4+)
    // Flags: GL_MAP_READ_BIT, GL_MAP_WRITE_BIT, GL_MAP_PERSISTENT_BIT, GL_MAP_COHERENT_BIT
    void CreatePersistentBuffer(uint32_t size, GLbitfield flags);

    // Bind to specific binding point (for shader access)
    void BindBase(uint32_t bindingPoint) const;

    // Update buffer data
    void SetData(const void* data, uint32_t size, uint32_t offset = 0);

    // Resize buffer (reallocates - NOT supported for persistent buffers)
    void Resize(uint32_t newSize);

    // Map buffer for direct CPU access (traditional mapping)
    void* Map(GLenum access = GL_READ_WRITE);
    void Unmap();

    // Access persistent pointer (only valid if created with CreatePersistentBuffer)
    void* GetPersistentPtr() const { return m_PersistentPtr; }
    bool IsPersistent() const { return m_IsPersistent; }

    // Read data back from GPU
    void GetData(void* data, uint32_t size, uint32_t offset = 0) const;

    // Memory barrier for synchronization
    void MemoryBarrier(GLbitfield barriers = GL_SHADER_STORAGE_BARRIER_BIT);

    // Getters
    uint32_t GetSSBOHandle() const { return m_SSBOHandle; }
    uint32_t GetSize() const { return m_Size; }
    GLenum GetUsage() const { return m_Usage; }
    
private:
    uint32_t m_SSBOHandle;
    uint32_t m_Size;
    GLenum m_Usage;
    mutable bool m_IsMapped;

    // Persistent mapping support (GL 4.4+)
    bool m_IsPersistent;
    void* m_PersistentPtr;
    GLbitfield m_PersistentFlags;
};

// Templated helper for structured data
template<typename T>
class TypedShaderStorageBuffer : public ShaderStorageBuffer
{
public:
    TypedShaderStorageBuffer(const std::vector<T>& data = {}, GLenum usage = GL_DYNAMIC_DRAW)
        : ShaderStorageBuffer(data.empty() ? nullptr : data.data(), 
                            static_cast<uint32_t>(data.size() * sizeof(T)), usage) {}
    
    void SetData(const std::vector<T>& data, uint32_t offset = 0) {
        ShaderStorageBuffer::SetData(data.data(), 
                                   static_cast<uint32_t>(data.size() * sizeof(T)), offset);
    }
    
    void SetElement(const T& element, uint32_t index) {
        ShaderStorageBuffer::SetData(&element, sizeof(T), index * sizeof(T));
    }
    
    void GetData(std::vector<T>& data) const {
        data.resize(GetElementCount());
        ShaderStorageBuffer::GetData(data.data(), GetSize(), 0);
    }
    
    T GetElement(uint32_t index) const {
        T element;
        ShaderStorageBuffer::GetData(&element, sizeof(T), index * sizeof(T));
        return element;
    }
    
    uint32_t GetElementCount() const { return GetSize() / sizeof(T); }
};