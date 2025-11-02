/******************************************************************************/
/*!
\file   MaterialPropertyBlock.cpp
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/22
\brief  Implements the MaterialPropertyBlock class for lightweight per-object
        material property overrides.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include <Resources/MaterialPropertyBlock.h>
#include <glad/glad.h>

// ========== PROPERTY SETTERS ==========

void MaterialPropertyBlock::SetFloat(const std::string& name, float value)
{
    m_Properties[name] = value;
}

void MaterialPropertyBlock::SetVec3(const std::string& name, const glm::vec3& value)
{
    m_Properties[name] = value;
}

void MaterialPropertyBlock::SetVec4(const std::string& name, const glm::vec4& value)
{
    m_Properties[name] = value;
}

void MaterialPropertyBlock::SetMat4(const std::string& name, const glm::mat4& value)
{
    m_Properties[name] = value;
}

void MaterialPropertyBlock::SetTexture(const std::string& name, std::shared_ptr<Texture> texture, int textureUnit)
{
    // Auto-assign texture unit if not specified
    if (textureUnit == -1)
    {
        textureUnit = m_NextTextureUnit++;
    }

    m_Properties[name] = std::make_pair(texture, textureUnit);
}

// ========== PROPERTY GETTERS ==========

bool MaterialPropertyBlock::TryGetFloat(const std::string& name, float& outValue) const
{
    auto it = m_Properties.find(name);
    if (it == m_Properties.end())
        return false;

    if (const float* value = std::get_if<float>(&it->second))
    {
        outValue = *value;
        return true;
    }
    return false;
}

bool MaterialPropertyBlock::TryGetVec3(const std::string& name, glm::vec3& outValue) const
{
    auto it = m_Properties.find(name);
    if (it == m_Properties.end())
        return false;

    if (const glm::vec3* value = std::get_if<glm::vec3>(&it->second))
    {
        outValue = *value;
        return true;
    }
    return false;
}

bool MaterialPropertyBlock::TryGetVec4(const std::string& name, glm::vec4& outValue) const
{
    auto it = m_Properties.find(name);
    if (it == m_Properties.end())
        return false;

    if (const glm::vec4* value = std::get_if<glm::vec4>(&it->second))
    {
        outValue = *value;
        return true;
    }
    return false;
}

bool MaterialPropertyBlock::TryGetMat4(const std::string& name, glm::mat4& outValue) const
{
    auto it = m_Properties.find(name);
    if (it == m_Properties.end())
        return false;

    if (const glm::mat4* value = std::get_if<glm::mat4>(&it->second))
    {
        outValue = *value;
        return true;
    }
    return false;
}

bool MaterialPropertyBlock::TryGetTexture(const std::string& name, std::shared_ptr<Texture>& outTexture, int& outTextureUnit) const
{
    auto it = m_Properties.find(name);
    if (it == m_Properties.end())
        return false;

    using TexturePair = std::pair<std::shared_ptr<Texture>, int>;
    if (const TexturePair* value = std::get_if<TexturePair>(&it->second))
    {
        outTexture = value->first;
        outTextureUnit = value->second;
        return true;
    }
    return false;
}

// ========== PROPERTY QUERIES ==========

bool MaterialPropertyBlock::HasProperty(const std::string& name) const
{
    return m_Properties.find(name) != m_Properties.end();
}

// ========== APPLICATION ==========

void MaterialPropertyBlock::ApplyToShader(std::shared_ptr<Shader> shader) const
{
    if (!shader)
        return;

    // Iterate through all properties and apply them to the shader
    for (const auto& [name, value] : m_Properties)
    {
        // Use std::visit to handle different property types
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, float>)
            {
                shader->setFloat(name, arg);
            }
            else if constexpr (std::is_same_v<T, glm::vec3>)
            {
                shader->setVec3(name, arg);
            }
            else if constexpr (std::is_same_v<T, glm::vec4>)
            {
                shader->setVec4(name, arg);
            }
            else if constexpr (std::is_same_v<T, glm::mat4>)
            {
                shader->setMat4(name, arg);
            }
            else if constexpr (std::is_same_v<T, std::pair<std::shared_ptr<Texture>, int>>)
            {
                // Texture property: bind texture and set sampler uniform
                const auto& [texture, unit] = arg;
                if (texture && texture->id != 0)
                {
                    // Activate texture unit
                    glActiveTexture(GL_TEXTURE0 + unit);
                    glBindTexture(GL_TEXTURE_2D, texture->id);

                    // Set sampler uniform to the texture unit
                    shader->setInt(name, unit);
                }
            }
        }, value);
    }
}

// ========== UTILITY ==========

void MaterialPropertyBlock::Clear()
{
    m_Properties.clear();
    m_NextTextureUnit = 0;
}

void MaterialPropertyBlock::CopyFrom(const MaterialPropertyBlock& other)
{
    m_Properties = other.m_Properties;
    m_NextTextureUnit = other.m_NextTextureUnit;
}
