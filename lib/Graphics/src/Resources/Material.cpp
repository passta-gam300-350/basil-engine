/******************************************************************************/
/*!
\file   Material.cpp
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Implementation of PBR material system with shader property management

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include <Resources/Material.h>
#include <spdlog/spdlog.h>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

Material::Material(const std::shared_ptr<Shader>& shader, const std::string& name)
    : m_Shader(shader), m_Name(name)
{
    if (!shader)
    {
        spdlog::warn("Material '{}' created with null shader!", name);
    }
}

GLint Material::GetUniformLocation(const std::string& name) const
{
    if (!m_Shader)
    {
        spdlog::warn("Material '{}': Cannot get uniform location '{}' - no shader assigned", m_Name, name);
        return -1;
    }

    // Check if location is already cached
    auto it = m_UniformLocationCache.find(name);
    if (it != m_UniformLocationCache.end())
    {
        return it->second;
    }

    // Not cached - query OpenGL and cache the result
    GLint location = glGetUniformLocation(m_Shader->ID, name.c_str());

    if (location == -1)
    {
        spdlog::warn("Material '{}': Uniform '{}' not found in shader (may be optimized out or doesn't exist)", m_Name, name);
    }

    // Cache the location (even if -1, to avoid repeated failed lookups)
    m_UniformLocationCache[name] = location;
    return location;
}

void Material::InvalidateCache()
{
    m_UniformLocationCache.clear();
}

void Material::SetShader(std::shared_ptr<Shader> shader)
{
    if (m_Shader != shader)
    {
        m_Shader = shader;
        InvalidateCache(); // Clear cache when shader changes

        // Re-apply all stored properties to the new shader
        if (m_Shader)
        {
            // Note: Shader must be bound before calling this
            // The calling code is responsible for binding the shader
            spdlog::info("Material '{}': Shader changed, re-applying {} stored properties",
                        m_Name, m_FloatProperties.size() + m_Vec3Properties.size() +
                        m_Vec4Properties.size() + m_Mat4Properties.size() + m_TextureProperties.size());
        }
        else
        {
            spdlog::info("Material '{}': Shader changed to null, uniform cache invalidated", m_Name);
        }
    }
}

void Material::SetFloat(const std::string& name, float value)
{
    // Dirty-checking: Only update if value changed
    auto it = m_FloatProperties.find(name);
    if (it != m_FloatProperties.end() && it->second == value)
    {
        return; // Value unchanged, skip GPU update
    }

    // Store the property locally
    m_FloatProperties[name] = value;

    // Apply to shader immediately if shader is available
    GLint location = GetUniformLocation(name);
    if (location != -1)
    {
        // Note: Shader is already bound by the command buffer system
        glUniform1f(location, value);
    }
}

void Material::SetVec3(const std::string& name, const glm::vec3& value)
{
    // Dirty-checking: Only update if value changed
    auto it = m_Vec3Properties.find(name);
    if (it != m_Vec3Properties.end() && it->second == value)
    {
        return; // Value unchanged, skip GPU update
    }

    // Store the property locally
    m_Vec3Properties[name] = value;

    // Apply to shader immediately if shader is available
    GLint location = GetUniformLocation(name);
    if (location != -1)
    {
        // Note: Shader is already bound by the command buffer system
        glUniform3fv(location, 1, &value[0]);
    }
}

void Material::SetVec4(const std::string& name, const glm::vec4& value)
{
    // Dirty-checking: Only update if value changed
    auto it = m_Vec4Properties.find(name);
    if (it != m_Vec4Properties.end() && it->second == value)
    {
        return; // Value unchanged, skip GPU update
    }

    // Store the property locally
    m_Vec4Properties[name] = value;

    // Apply to shader immediately if shader is available
    GLint location = GetUniformLocation(name);
    if (location != -1)
    {
        // Note: Shader is already bound by the command buffer system
        glUniform4fv(location, 1, &value[0]);
    }
}

void Material::SetMat4(const std::string& name, const glm::mat4& value)
{
    // Dirty-checking: Only update if value changed
    auto it = m_Mat4Properties.find(name);
    if (it != m_Mat4Properties.end() && it->second == value)
    {
        return; // Value unchanged, skip GPU update
    }

    // Store the property locally
    m_Mat4Properties[name] = value;

    // Apply to shader immediately if shader is available
    GLint location = GetUniformLocation(name);
    if (location != -1)
    {
        // Note: Shader is already bound by the command buffer system
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

void Material::SetTexture(const std::string& name, std::shared_ptr<Texture> texture, int textureUnit)
{
    // Determine the texture unit to use
    int unitToUse = textureUnit;
    if (unitToUse == -1) {
        // Auto-assign: Find existing texture or use next available unit
        auto it = m_TextureProperties.find(name);
        if (it != m_TextureProperties.end()) {
            // Texture already exists for this name, reuse its unit
            unitToUse = it->second.second;
        } else {
            // Assign new unit
            unitToUse = m_NextTextureUnit++;
        }
    }

    // Dirty-checking: Only update if texture or unit changed
    auto it = m_TextureProperties.find(name);
    if (it != m_TextureProperties.end() &&
        it->second.first == texture &&
        it->second.second == unitToUse)
    {
        return; // Texture and unit unchanged, skip GPU update
    }

    // Store the texture property
    m_TextureProperties[name] = std::make_pair(texture, unitToUse);

    // Apply to shader immediately if shader is available and texture is valid
    if (texture && texture->id != 0)
    {
        GLint location = GetUniformLocation(name);
        if (location != -1)
        {
            // Bind texture to the assigned unit
            glActiveTexture(GL_TEXTURE0 + unitToUse);
            glBindTexture(texture->target, texture->id);

            // Set the sampler uniform to the texture unit
            glUniform1i(location, unitToUse);
        }
    }
}

void Material::ApplyPBRProperties()
{
    if (!m_Shader) {
        return;
    }

    // Note: Shader is already bound by the command buffer system, don't call use() here

    // Apply PBR material properties to shader using cached uniform locations
    SetVec3("u_AlbedoColor", m_AlbedoColor);
    SetFloat("u_MetallicValue", m_MetallicValue);
    SetFloat("u_RoughnessValue", m_RoughnessValue);

    // Note: Texture flags (u_HasDiffuseMap, etc.) are handled by bindless texture system
    // The bindless system uploads texture handles to SSBO and sets the appropriate flags
}

void Material::SetAlbedoColorSRGB(const glm::vec3& srgbColor) {
    // Simple sRGB to linear conversion using gamma 2.2 approximation
    m_AlbedoColor = glm::pow(srgbColor, glm::vec3(2.2f));
}

// ============================================================================
// Property Storage System - Getters
// ============================================================================

bool Material::HasFloatProperty(const std::string& name) const
{
    return m_FloatProperties.find(name) != m_FloatProperties.end();
}

bool Material::HasVec3Property(const std::string& name) const
{
    return m_Vec3Properties.find(name) != m_Vec3Properties.end();
}

bool Material::HasVec4Property(const std::string& name) const
{
    return m_Vec4Properties.find(name) != m_Vec4Properties.end();
}

bool Material::HasMat4Property(const std::string& name) const
{
    return m_Mat4Properties.find(name) != m_Mat4Properties.end();
}

bool Material::HasTexture(const std::string& name) const
{
    return m_TextureProperties.find(name) != m_TextureProperties.end();
}

float Material::GetFloatProperty(const std::string& name, float defaultValue) const
{
    auto it = m_FloatProperties.find(name);
    return (it != m_FloatProperties.end()) ? it->second : defaultValue;
}

glm::vec3 Material::GetVec3Property(const std::string& name, const glm::vec3& defaultValue) const
{
    auto it = m_Vec3Properties.find(name);
    return (it != m_Vec3Properties.end()) ? it->second : defaultValue;
}

glm::vec4 Material::GetVec4Property(const std::string& name, const glm::vec4& defaultValue) const
{
    auto it = m_Vec4Properties.find(name);
    return (it != m_Vec4Properties.end()) ? it->second : defaultValue;
}

glm::mat4 Material::GetMat4Property(const std::string& name, const glm::mat4& defaultValue) const
{
    auto it = m_Mat4Properties.find(name);
    return (it != m_Mat4Properties.end()) ? it->second : defaultValue;
}

std::shared_ptr<Texture> Material::GetTexture(const std::string& name) const
{
    auto it = m_TextureProperties.find(name);
    return (it != m_TextureProperties.end()) ? it->second.first : nullptr;
}

// ============================================================================
// Apply All Properties - Sync stored properties to shader
// ============================================================================

void Material::ApplyAllProperties()
{
    if (!m_Shader)
    {
        spdlog::warn("Material '{}': Cannot apply properties - no shader assigned", m_Name);
        return;
    }

    // Apply all stored float properties
    for (const auto& [name, value] : m_FloatProperties)
    {
        GLint location = GetUniformLocation(name);
        if (location != -1)
        {
            glUniform1f(location, value);
        }
    }

    // Apply all stored vec3 properties
    for (const auto& [name, value] : m_Vec3Properties)
    {
        GLint location = GetUniformLocation(name);
        if (location != -1)
        {
            glUniform3fv(location, 1, &value[0]);
        }
    }

    // Apply all stored vec4 properties
    for (const auto& [name, value] : m_Vec4Properties)
    {
        GLint location = GetUniformLocation(name);
        if (location != -1)
        {
            glUniform4fv(location, 1, &value[0]);
        }
    }

    // Apply all stored mat4 properties
    for (const auto& [name, value] : m_Mat4Properties)
    {
        GLint location = GetUniformLocation(name);
        if (location != -1)
        {
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }

    // Apply all stored texture properties
    for (const auto& [name, texturePair] : m_TextureProperties)
    {
        const auto& [texture, unit] = texturePair;
        if (texture && texture->id != 0)
        {
            GLint location = GetUniformLocation(name);
            if (location != -1)
            {
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(texture->target, texture->id);
                glUniform1i(location, unit);
            }
        }
    }

    spdlog::debug("Material '{}': Applied {} float, {} vec3, {} vec4, {} mat4, {} texture properties",
                  m_Name, m_FloatProperties.size(), m_Vec3Properties.size(),
                  m_Vec4Properties.size(), m_Mat4Properties.size(), m_TextureProperties.size());
}