#include <Resources/Material.h>
#include <spdlog/spdlog.h>

Material::Material(std::shared_ptr<Shader> shader, const std::string& name)
    : m_Shader(shader), m_Name(name)
{
    if (!shader)
    {
        spdlog::warn("Material '{}' created with null shader!", name);
    }
}

void Material::SetFloat(const std::string& name, float value)
{
    if (m_Shader)
    {
        m_Shader->use();
        m_Shader->setFloat(name, value);
    }
}

void Material::SetVec3(const std::string& name, const glm::vec3& value)
{
    if (m_Shader)
    {
        m_Shader->use();
        m_Shader->setVec3(name, value);
    }
}

void Material::SetVec4(const std::string& name, const glm::vec4& value)
{
    if (m_Shader)
    {
        m_Shader->use();
        m_Shader->setVec4(name, value);
    }
}

void Material::SetMat4(const std::string& name, const glm::mat4& value)
{
    if (m_Shader)
    {
        m_Shader->use();
        m_Shader->setMat4(name, value);
    }
}

void Material::ApplyPBRProperties()
{
    if (!m_Shader) return;

    // Note: Shader is already bound by the command buffer system, don't call use() here

    // Apply PBR material properties to shader
    m_Shader->setVec3("u_AlbedoColor", m_AlbedoColor);
    m_Shader->setFloat("u_MetallicValue", m_MetallicValue);
    m_Shader->setFloat("u_RoughnessValue", m_RoughnessValue);

    // Note: Texture flags (u_HasDiffuseMap, etc.) are handled by bindless texture system
    // The bindless system uploads texture handles to SSBO and sets the appropriate flags
}