/******************************************************************************/
/*!
\file   UniformBuffer.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the UniformBuffer class for OpenGL uniform buffer object (UBO) management

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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