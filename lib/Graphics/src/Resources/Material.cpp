#include <Resources/Material.h>
#include <iostream>

Material::Material(std::shared_ptr<Shader> shader, const std::string& name)
    : m_Shader(shader), m_Name(name)
{
    if (!shader)
    {
        std::cerr << "Warning: Material '" << name << "' created with null shader!" << std::endl;
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