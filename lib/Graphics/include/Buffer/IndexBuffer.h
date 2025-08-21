#pragma once

#include <cstdint>

class IndexBuffer
{
public:
    IndexBuffer(const uint32_t *indices, uint32_t count);
    ~IndexBuffer();

    void Bind() const;
    void Unbind() const;

    uint32_t GetCount() const
    {
        return m_Count;
    }

    uint32_t GetVAOHandle() const
    {
        return m_IBOHandle;
    }

private:
    uint32_t m_IBOHandle;
    uint32_t m_Count;
};