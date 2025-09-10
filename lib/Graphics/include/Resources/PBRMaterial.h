#pragma once

#include <Resources/Material.h>
#include <Resources/Texture.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

/**
 * Enhanced Material class specifically for PBR (Physically Based Rendering) workflows
 * Provides convenient methods for setting PBR properties and loading PBR texture sets
 */
class PBRMaterial : public Material
{
public:
    struct PBRProperties {
        // Base PBR properties
        glm::vec3 albedo = glm::vec3(0.8f, 0.8f, 0.8f);  // Base color/diffuse
        float metallic = 0.0f;                             // 0.0 = dielectric, 1.0 = metallic
        float roughness = 0.5f;                           // 0.0 = mirror, 1.0 = completely rough
        float ao = 1.0f;                                  // Ambient occlusion multiplier
        glm::vec3 emissive = glm::vec3(0.0f);             // Self-illumination color
        
        // Advanced PBR properties (for future IBL implementation)
        float clearcoat = 0.0f;                           // Clearcoat layer strength
        float clearcoatRoughness = 0.0f;                  // Clearcoat roughness
        float transmission = 0.0f;                         // Transmission/transparency
        float ior = 1.5f;                                 // Index of refraction
        
        // Texture scaling and adjustments
        float normalScale = 1.0f;                         // Normal map intensity
        float heightScale = 0.02f;                        // Height/parallax intensity
        float emissiveStrength = 1.0f;                    // Emissive intensity multiplier
    };

public:
    PBRMaterial(std::shared_ptr<Shader> shader, const std::string& name = "PBRMaterial");
    ~PBRMaterial() = default;

    // PBR property management
    void SetPBRProperties(const PBRProperties& properties);
    const PBRProperties& GetPBRProperties() const { return m_PBRProperties; }
    
    // Individual property setters (convenience methods)
    void SetAlbedo(const glm::vec3& albedo);
    void SetMetallic(float metallic);
    void SetRoughness(float roughness);
    void SetAO(float ao);
    void SetEmissive(const glm::vec3& emissive);
    void SetNormalScale(float scale);
    
    // Texture management
    void LoadPBRTextures(const std::string& basePath);
    void LoadPBRTexturesFromFileNames(const std::string& directory, 
                                      const std::string& baseColorPath = "",
                                      const std::string& normalPath = "",
                                      const std::string& metallicPath = "",
                                      const std::string& roughnessPath = "",
                                      const std::string& aoPath = "",
                                      const std::string& emissivePath = "",
                                      const std::string& heightPath = "");
    
    // Texture access
    bool HasTexture(const std::string& type) const;
    Texture* GetTexture(const std::string& type);
    const std::vector<Texture>& GetAllTextures() const { return m_Textures; }
    
    // Material presets
    static PBRMaterial CreateMetalMaterial(std::shared_ptr<Shader> shader, 
                                           const glm::vec3& baseColor = glm::vec3(0.7f, 0.7f, 0.7f),
                                           float roughness = 0.1f);
    static PBRMaterial CreatePlasticMaterial(std::shared_ptr<Shader> shader,
                                             const glm::vec3& baseColor = glm::vec3(0.8f, 0.2f, 0.2f),
                                             float roughness = 0.3f);
    static PBRMaterial CreateDielectricMaterial(std::shared_ptr<Shader> shader,
                                                const glm::vec3& baseColor = glm::vec3(0.6f, 0.6f, 0.6f),
                                                float roughness = 0.7f);
    
    // Update shader uniforms with current properties
    void UpdateShaderUniforms();
    
    // Debug information
    void PrintMaterialInfo() const;

private:
    PBRProperties m_PBRProperties;
    std::vector<Texture> m_Textures;
    std::unordered_map<std::string, size_t> m_TextureIndices; // Maps texture type to index in m_Textures
    
    // Helper methods
    void UpdatePropertyUniforms();
    void UpdateTextureFlags();
    Texture LoadTextureIfExists(const std::string& path, const std::string& type);
    std::string FindTextureFile(const std::string& directory, const std::vector<std::string>& possibleNames);
};