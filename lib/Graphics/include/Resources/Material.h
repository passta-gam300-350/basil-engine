#pragma once

#include <Resources/Shader.h>
#include <Resources/Texture.h>
#include <memory>
#include <vector>
#include <string>

// Simplified Material class - mostly just holds a shader reference and some basic properties
class Material
{
public:
    Material(std::shared_ptr<Shader> shader, const std::string& name = "Material");
    ~Material() = default;

    // Basic getters
    std::shared_ptr<Shader> GetShader() const { return m_Shader; }
    const std::string& GetName() const { return m_Name; }

    // Simple property setters that forward to the shader
    void SetFloat(const std::string& name, float value);
    void SetVec3(const std::string& name, const glm::vec3& value);
    void SetVec4(const std::string& name, const glm::vec4& value);
    void SetMat4(const std::string& name, const glm::mat4& value);

private:
    std::shared_ptr<Shader> m_Shader;
    std::string m_Name;
};