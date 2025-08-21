#pragma once

#include <Resources/Shader.h>
#include <Resources/Texture.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <string>

#include <assimp/material.h>

class Material
{
public:
    // Constructor
    Material(std::shared_ptr<Shader> const &shader, std::string const &name = "Material");
    ~Material() = default;

    // Binding for rendering
    void Bind();
    void Unbind();

    // Property setters (for shader uniforms)
    void SetFloat(std::string const &name, float value);
    void SetInt(std::string const &name, int value);
    void SetBool(std::string const &name, bool value);
    void SetVec2(std::string const &name, glm::vec2 const &value);
    void SetVec3(std::string const &name, glm::vec3 const &value);
    void SetVec4(std::string const &name, glm::vec4 const &value);
    void SetMat3(std::string const &name, glm::mat3 const &value);
    void SetMat4(std::string const &name, glm::mat4 const &value);

    // Texture management
    void SetTexture(std::string const &uniformName, std::shared_ptr<Texture> const &texture);
    void SetTexture(Texture::Type type, std::shared_ptr<Texture> const &texture);
    void RemoveTexture(std::string const &uniformName);
    void RemoveTexture(Texture::Type type);

    // Common material properties (PBR-ready)
    void SetAlbedo(glm::vec3 const &color)
    {
        SetVec3("u_Material.albedo", color);
    }
    void SetMetallic(float metallic)
    {
        SetFloat("u_Material.metallic", metallic);
    }
    void SetRoughness(float roughness)
    {
        SetFloat("u_Material.roughness", roughness);
    }
    void SetAO(float ao)
    {
        SetFloat("u_Material.ao", ao);
    }
    void SetEmission(glm::vec3 const &emission)
    {
        SetVec3("u_Material.emission", emission);
    }

    // Getters
    std::shared_ptr<Shader> GetShader() const
    {
        return m_Shader;
    }
    const std::string &GetName() const
    {
        return m_Name;
    }

    // Texture queries
    bool HasTexture(std::string const &uniformName) const;
    bool HasTexture(Texture::Type type) const;
    std::shared_ptr<Texture> GetTexture(std::string const &uniformName) const;
    std::shared_ptr<Texture> GetTexture(Texture::Type type) const;

    // Get all textures
    std::unordered_map<std::string, std::shared_ptr<Texture>> GetTextures() const;

    // Assimp integration
    void LoadFromAssimp(aiMaterial *assimpMaterial, std::string const &directory);
    static Texture::Type AssimpTextureTypeToEngineType(aiTextureType assimpType);


private:
    struct TextureInfo
    {
        std::shared_ptr<Texture> texture;
        uint32_t textureUnit;
    };

    void UpdateTextureUnits();
    std::string GetUniformNameForTextureType(Texture::Type type) const;

    // Assimp helper methods
    void LoadTexturesFromAssimp(aiMaterial *material, std::string const &directory, aiTextureType textureType);

    std::shared_ptr<Shader> m_Shader;
    std::string m_Name;

    // Texture management
    std::unordered_map<std::string, TextureInfo> m_Textures;
    std::unordered_map<Texture::Type, std::string> m_TextureTypeToUniform;
    uint32_t m_NextTextureUnit = 0;

    // Property caching (useful for debugging and Assimp)
    std::unordered_map<std::string, float> m_FloatProperties;
    std::unordered_map<std::string, glm::vec3> m_Vec3Properties;
    std::unordered_map<std::string, glm::vec4> m_Vec4Properties;

};