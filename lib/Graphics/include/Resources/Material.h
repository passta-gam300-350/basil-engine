#pragma once

#include <Resources/Shader.h>
#include <Resources/Texture.h>
#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>

// Unified Material class with PBR properties
class Material
{
public:
    Material(std::shared_ptr<Shader> shader, const std::string& name = "Material");
    ~Material() = default;

    // Basic getters
    std::shared_ptr<Shader> GetShader() const { return m_Shader; }
    const std::string& GetName() const { return m_Name; }

    // Shader setters
    void SetShader(std::shared_ptr<Shader> shader) { m_Shader = shader; }

    // Generic property setters that forward to the shader
    void SetFloat(const std::string& name, float value);
    void SetVec3(const std::string& name, const glm::vec3& value);
    void SetVec4(const std::string& name, const glm::vec4& value);
    void SetMat4(const std::string& name, const glm::mat4& value);

    // PBR Properties - both getters and setters
    const glm::vec3& GetAlbedoColor() const { return m_AlbedoColor; }
    float GetMetallicValue() const { return m_MetallicValue; }
    float GetRoughnessValue() const { return m_RoughnessValue; }

    void SetAlbedoColor(const glm::vec3& color) { m_AlbedoColor = color; }
    void SetAlbedoColorSRGB(const glm::vec3& srgbColor);
    void SetMetallicValue(float metallic) { m_MetallicValue = metallic; }
    void SetRoughnessValue(float roughness) { m_RoughnessValue = roughness; }

    // Apply all PBR properties to the shader at once
    void ApplyPBRProperties();

private:
    std::shared_ptr<Shader> m_Shader;
    std::string m_Name;

    // PBR Material Properties (previously in PBRMaterialProperties)
    glm::vec3 m_AlbedoColor = glm::vec3(0.8f, 0.7f, 0.6f);
    float m_MetallicValue = 0.7f;
    float m_RoughnessValue = 0.3f;
};