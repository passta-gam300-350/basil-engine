#pragma once

#include <cstdint>
#include <glm/glm.hpp>

class UniformBuffer
{
public:
	UniformBuffer(uint32_t size, uint32_t binding);
	~UniformBuffer();

    void SetData(const void *data, uint32_t size, uint32_t offset = 0) const;

    template<typename T>
    void Set(uint32_t offset, const T &data);

    void Bind() const;
    void Unbind() const;

private:
	uint32_t m_UBOHandle;
	uint32_t m_Binding;
};

template<typename T>
inline void UniformBuffer::Set(uint32_t offset, const T& data)
{
    SetData(&data, sizeof(T), offset);
}

template<>
inline void UniformBuffer::Set<float>(uint32_t offset, const float &data)
{
    SetData(&data, sizeof(float), offset);
}

template<>
inline void UniformBuffer::Set<glm::vec2>(uint32_t offset, const glm::vec2 &data)
{
    SetData(&data, sizeof(glm::vec2), offset);
}

template<>
inline void UniformBuffer::Set<glm::vec3>(uint32_t offset, const glm::vec3 &data)
{
    SetData(&data, sizeof(glm::vec3), offset);
}

template<>
inline void UniformBuffer::Set<glm::vec4>(uint32_t offset, const glm::vec4 &data)
{
    SetData(&data, sizeof(glm::vec4), offset);
}

template<>
inline void UniformBuffer::Set<glm::mat4>(uint32_t offset, const glm::mat4 &data)
{
    SetData(&data, sizeof(glm::mat4), offset);
}