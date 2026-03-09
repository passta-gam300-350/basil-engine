/******************************************************************************/
/*!
\file   Material.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the Material class for managing shader and PBR material properties

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <Resources/Shader.h>
#include <Resources/Texture.h>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glad/glad.h>

// Blend modes for transparency rendering
enum class BlendingMode
{
    Opaque = 0,      // Fully opaque, no blending
    Transparent = 1  // Alpha blending enabled
};

// Unified Material class with PBR properties
class Material
{
public:
    Material(const std::shared_ptr<Shader>& shader, const std::string& name = "Material");
    ~Material() = default;

    // Basic getters
    std::shared_ptr<Shader> GetShader() const { return m_Shader; }
    const std::string& GetName() const { return m_Name; }

    // Shader setters
    void SetShader(std::shared_ptr<Shader> shader);

    // Generic property setters that forward to the shader
    void SetFloat(const std::string& name, float value);
    void SetVec3(const std::string& name, const glm::vec3& value);
    void SetVec4(const std::string& name, const glm::vec4& value);
    void SetMat4(const std::string& name, const glm::mat4& value);
    void SetTexture(const std::string& name, std::shared_ptr<Texture> texture, int textureUnit = -1);

    // PBR Properties - both getters and setters
    const glm::vec3& GetAlbedoColor() const { return m_AlbedoColor; }
    float GetMetallicValue() const { return m_MetallicValue; }
    float GetRoughnessValue() const { return m_RoughnessValue; }
    float GetNormalStrength() const { return m_NormalStrength; }
    BlendingMode GetBlendMode() const { return m_BlendMode; }

    void SetAlbedoColor(const glm::vec3& color) { m_AlbedoColor = color; }
    void SetAlbedoColorSRGB(const glm::vec3& srgbColor);
    void SetMetallicValue(float metallic) { m_MetallicValue = metallic; }
    void SetRoughnessValue(float roughness) { m_RoughnessValue = roughness; }
    void SetNormalStrength(float strength) { m_NormalStrength = strength; }
    void SetBlendMode(BlendingMode mode) { m_BlendMode = mode; }

    // Apply all PBR properties to the shader at once
    void ApplyPBRProperties();

    // Apply all stored properties to the shader (for property storage system)
    void ApplyAllProperties();

    // Property getters for stored values
    bool HasFloatProperty(const std::string& name) const;
    bool HasVec3Property(const std::string& name) const;
    bool HasVec4Property(const std::string& name) const;
    bool HasMat4Property(const std::string& name) const;
    bool HasTexture(const std::string& name) const;

    float GetFloatProperty(const std::string& name, float defaultValue = 0.0f) const;
    glm::vec3 GetVec3Property(const std::string& name, const glm::vec3& defaultValue = glm::vec3(0.0f)) const;
    glm::vec4 GetVec4Property(const std::string& name, const glm::vec4& defaultValue = glm::vec4(0.0f)) const;
    glm::mat4 GetMat4Property(const std::string& name, const glm::mat4& defaultValue = glm::mat4(1.0f)) const;
    std::shared_ptr<Texture> GetTexture(const std::string& name) const;

    // Get all textures as a vector (for rendering pipeline)
    // Converts uniform names (u_DiffuseMap) to texture types (texture_diffuse)
    std::vector<Texture> GetAllTextures() const;

private:
    // Helper method to get cached uniform location with validation
    GLint GetUniformLocation(const std::string& name) const;

    // Clear the uniform cache (call when shader changes)
    void InvalidateCache();

    std::shared_ptr<Shader> m_Shader;
    std::string m_Name;

    // PBR Material Properties (previously in PBRMaterialProperties)
    glm::vec3 m_AlbedoColor = glm::vec3(0.8f, 0.7f, 0.6f);
    float m_MetallicValue = 0.7f;
    float m_RoughnessValue = 0.3f;
    float m_NormalStrength = 1.0f;
    BlendingMode m_BlendMode = BlendingMode::Opaque;

    // Performance optimization: Cache uniform locations to avoid repeated glGetUniformLocation calls
    // Using mutable to allow caching in const methods
    // Map: uniform name -> OpenGL location (-1 if not found)
    mutable std::unordered_map<std::string, GLint> m_UniformLocationCache;

    // Property storage for serialization and dirty-checking
    // These store the "source of truth" for material properties
    std::unordered_map<std::string, float> m_FloatProperties;
    std::unordered_map<std::string, glm::vec3> m_Vec3Properties;
    std::unordered_map<std::string, glm::vec4> m_Vec4Properties;
    std::unordered_map<std::string, glm::mat4> m_Mat4Properties;

    // Texture properties: stores texture + assigned texture unit
    // Texture unit assignment: -1 means auto-assign, >= 0 is explicit unit
    std::unordered_map<std::string, std::pair<std::shared_ptr<Texture>, int>> m_TextureProperties;
    mutable int m_NextTextureUnit = 0; // Counter for auto-assignment
};