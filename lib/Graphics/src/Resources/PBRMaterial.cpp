#include <Resources/PBRMaterial.h>
#include <iostream>
#include <filesystem>
#include <algorithm>

PBRMaterial::PBRMaterial(std::shared_ptr<Shader> shader, const std::string& name)
    : Material(shader, name)
{
    // Initialize with default PBR properties
    UpdateShaderUniforms();
}

void PBRMaterial::SetPBRProperties(const PBRProperties& properties)
{
    m_PBRProperties = properties;
    UpdatePropertyUniforms();
}

void PBRMaterial::SetAlbedo(const glm::vec3& albedo)
{
    m_PBRProperties.albedo = albedo;
    if (GetShader()) {
        GetShader()->use();
        GetShader()->setVec3("u_AlbedoColor", albedo);
    }
}

void PBRMaterial::SetMetallic(float metallic)
{
    m_PBRProperties.metallic = glm::clamp(metallic, 0.0f, 1.0f);
    if (GetShader()) {
        GetShader()->use();
        GetShader()->setFloat("u_Metallic", m_PBRProperties.metallic);
    }
}

void PBRMaterial::SetRoughness(float roughness)
{
    m_PBRProperties.roughness = glm::clamp(roughness, 0.0f, 1.0f);
    if (GetShader()) {
        GetShader()->use();
        GetShader()->setFloat("u_Roughness", m_PBRProperties.roughness);
    }
}

void PBRMaterial::SetAO(float ao)
{
    m_PBRProperties.ao = glm::clamp(ao, 0.0f, 1.0f);
    if (GetShader()) {
        GetShader()->use();
        GetShader()->setFloat("u_AO", m_PBRProperties.ao);
    }
}

void PBRMaterial::SetEmissive(const glm::vec3& emissive)
{
    m_PBRProperties.emissive = emissive;
    if (GetShader()) {
        GetShader()->use();
        GetShader()->setVec3("u_EmissiveColor", emissive);
    }
}

void PBRMaterial::SetNormalScale(float scale)
{
    m_PBRProperties.normalScale = scale;
    if (GetShader()) {
        GetShader()->use();
        GetShader()->setFloat("u_NormalScale", scale);
    }
}

void PBRMaterial::LoadPBRTextures(const std::string& basePath)
{
    std::cout << "Loading PBR texture set from: " << basePath << std::endl;
    
    // Common PBR texture naming conventions
    std::vector<std::pair<std::string, std::vector<std::string>>> texturePatterns = {
        {"texture_diffuse", {basePath + "_BaseColor", basePath + "_Diffuse", basePath + "_Albedo", basePath + "_Color"}},
        {"texture_normal", {basePath + "_Normal", basePath + "_NormalMap", basePath + "_Norm"}},
        {"texture_metallic", {basePath + "_Metallic", basePath + "_Metal", basePath + "_Metalness"}},
        {"texture_roughness", {basePath + "_Roughness", basePath + "_Rough"}},
        {"texture_ao", {basePath + "_AO", basePath + "_AmbientOcclusion", basePath + "_Occlusion"}},
        {"texture_emissive", {basePath + "_Emissive", basePath + "_Emission", basePath + "_Glow"}},
        {"texture_height", {basePath + "_Height", basePath + "_Displacement", basePath + "_Disp"}}
    };
    
    m_Textures.clear();
    m_TextureIndices.clear();
    
    // Try to load each texture type
    for (const auto& [textureType, patterns] : texturePatterns) {
        bool found = false;
        
        for (const std::string& pattern : patterns) {
            // Try common image extensions
            std::vector<std::string> extensions = {".png", ".jpg", ".jpeg", ".tga", ".bmp", ".hdr", ".exr"};
            
            for (const std::string& ext : extensions) {
                std::string fullPath = pattern + ext;
                
                if (std::filesystem::exists(fullPath)) {
                    Texture texture = LoadTextureIfExists(fullPath, textureType);
                    if (texture.id != 0) {
                        m_TextureIndices[textureType] = m_Textures.size();
                        m_Textures.push_back(texture);
                        std::cout << "✓ Loaded " << textureType << ": " << fullPath << std::endl;
                        found = true;
                        break;
                    }
                }
            }
            
            if (found) break;
        }
        
        if (!found) {
            std::cout << "⚠ No texture found for " << textureType << std::endl;
        }
    }
    
    // Update texture availability flags in shader
    UpdateTextureFlags();
    
    std::cout << "PBR texture loading complete. Loaded " << m_Textures.size() << " textures." << std::endl;
}

void PBRMaterial::LoadPBRTexturesFromFileNames(const std::string& directory,
                                               const std::string& baseColorPath,
                                               const std::string& normalPath,
                                               const std::string& metallicPath,
                                               const std::string& roughnessPath,
                                               const std::string& aoPath,
                                               const std::string& emissivePath,
                                               const std::string& heightPath)
{
    m_Textures.clear();
    m_TextureIndices.clear();
    
    struct TextureInfo {
        std::string type;
        std::string path;
    };
    
    std::vector<TextureInfo> texturesToLoad = {
        {"texture_diffuse", baseColorPath},
        {"texture_normal", normalPath},
        {"texture_metallic", metallicPath},
        {"texture_roughness", roughnessPath},
        {"texture_ao", aoPath},
        {"texture_emissive", emissivePath},
        {"texture_height", heightPath}
    };
    
    for (const auto& info : texturesToLoad) {
        if (!info.path.empty()) {
            std::string fullPath = directory.empty() ? info.path : directory + "/" + info.path;
            
            if (std::filesystem::exists(fullPath)) {
                Texture texture = LoadTextureIfExists(fullPath, info.type);
                if (texture.id != 0) {
                    m_TextureIndices[info.type] = m_Textures.size();
                    m_Textures.push_back(texture);
                    std::cout << "✓ Loaded " << info.type << ": " << fullPath << std::endl;
                }
            } else {
                std::cout << "⚠ Texture file not found: " << fullPath << std::endl;
            }
        }
    }
    
    UpdateTextureFlags();
}

bool PBRMaterial::HasTexture(const std::string& type) const
{
    return m_TextureIndices.find(type) != m_TextureIndices.end();
}

Texture* PBRMaterial::GetTexture(const std::string& type)
{
    auto it = m_TextureIndices.find(type);
    if (it != m_TextureIndices.end() && it->second < m_Textures.size()) {
        return &m_Textures[it->second];
    }
    return nullptr;
}

PBRMaterial PBRMaterial::CreateMetalMaterial(std::shared_ptr<Shader> shader, const glm::vec3& baseColor, float roughness)
{
    PBRMaterial material(shader, "MetalMaterial");
    
    PBRProperties props;
    props.albedo = baseColor;
    props.metallic = 1.0f;  // Fully metallic
    props.roughness = roughness;
    props.ao = 1.0f;
    props.emissive = glm::vec3(0.0f);
    
    material.SetPBRProperties(props);
    return material;
}

PBRMaterial PBRMaterial::CreatePlasticMaterial(std::shared_ptr<Shader> shader, const glm::vec3& baseColor, float roughness)
{
    PBRMaterial material(shader, "PlasticMaterial");
    
    PBRProperties props;
    props.albedo = baseColor;
    props.metallic = 0.0f;  // Non-metallic
    props.roughness = roughness;
    props.ao = 1.0f;
    props.emissive = glm::vec3(0.0f);
    
    material.SetPBRProperties(props);
    return material;
}

PBRMaterial PBRMaterial::CreateDielectricMaterial(std::shared_ptr<Shader> shader, const glm::vec3& baseColor, float roughness)
{
    PBRMaterial material(shader, "DielectricMaterial");
    
    PBRProperties props;
    props.albedo = baseColor;
    props.metallic = 0.0f;  // Non-metallic
    props.roughness = roughness;
    props.ao = 1.0f;
    props.emissive = glm::vec3(0.0f);
    
    material.SetPBRProperties(props);
    return material;
}

void PBRMaterial::UpdateShaderUniforms()
{
    if (!GetShader()) {
        std::cerr << "Warning: Cannot update shader uniforms - no shader attached to PBRMaterial" << std::endl;
        return;
    }
    
    GetShader()->use();
    
    // Update property uniforms
    UpdatePropertyUniforms();
    
    // Update texture flags
    UpdateTextureFlags();
}

void PBRMaterial::UpdatePropertyUniforms()
{
    if (!GetShader()) return;
    
    auto shader = GetShader();
    shader->use();
    
    // Set PBR properties
    shader->setVec3("u_AlbedoColor", m_PBRProperties.albedo);
    shader->setFloat("u_Metallic", m_PBRProperties.metallic);
    shader->setFloat("u_Roughness", m_PBRProperties.roughness);
    shader->setFloat("u_AO", m_PBRProperties.ao);
    shader->setVec3("u_EmissiveColor", m_PBRProperties.emissive);
    
    // Advanced properties (for future use)
    shader->setFloat("u_NormalScale", m_PBRProperties.normalScale);
    shader->setFloat("u_HeightScale", m_PBRProperties.heightScale);
    shader->setFloat("u_EmissiveStrength", m_PBRProperties.emissiveStrength);
}

void PBRMaterial::UpdateTextureFlags()
{
    if (!GetShader()) return;
    
    auto shader = GetShader();
    shader->use();
    
    // Set texture availability flags
    shader->setBool("u_HasDiffuseMap", HasTexture("texture_diffuse"));
    shader->setBool("u_HasNormalMap", HasTexture("texture_normal"));
    shader->setBool("u_HasMetallicMap", HasTexture("texture_metallic"));
    shader->setBool("u_HasRoughnessMap", HasTexture("texture_roughness"));
    shader->setBool("u_HasAOMap", HasTexture("texture_ao"));
    shader->setBool("u_HasEmissiveMap", HasTexture("texture_emissive"));
}

void PBRMaterial::PrintMaterialInfo() const
{
    std::cout << "\n=== PBR Material Info: " << GetName() << " ===" << std::endl;
    std::cout << "Albedo: (" << m_PBRProperties.albedo.x << ", " << m_PBRProperties.albedo.y << ", " << m_PBRProperties.albedo.z << ")" << std::endl;
    std::cout << "Metallic: " << m_PBRProperties.metallic << std::endl;
    std::cout << "Roughness: " << m_PBRProperties.roughness << std::endl;
    std::cout << "AO: " << m_PBRProperties.ao << std::endl;
    std::cout << "Emissive: (" << m_PBRProperties.emissive.x << ", " << m_PBRProperties.emissive.y << ", " << m_PBRProperties.emissive.z << ")" << std::endl;
    
    std::cout << "\nTextures (" << m_Textures.size() << " loaded):" << std::endl;
    for (const auto& [type, index] : m_TextureIndices) {
        if (index < m_Textures.size()) {
            const auto& texture = m_Textures[index];
            std::cout << "  " << type << " -> ID:" << texture.id << " (" << texture.path << ")" << std::endl;
        }
    }
    std::cout << "===========================\n" << std::endl;
}

Texture PBRMaterial::LoadTextureIfExists(const std::string& path, const std::string& type)
{
    // This is a placeholder - you would implement actual texture loading here
    // For now, we'll return a dummy texture structure
    Texture texture;
    texture.id = 0;  // This would be set by actual OpenGL texture loading
    texture.type = type;
    texture.path = path;
    
    // In a real implementation, you would:
    // 1. Load the image file using STB or similar
    // 2. Create OpenGL texture with glGenTextures
    // 3. Upload texture data with glTexImage2D
    // 4. Set up texture parameters
    // 5. Return texture with valid ID
    
    std::cout << "⚠ LoadTextureIfExists: Placeholder implementation - would load: " << path << std::endl;
    
    return texture;
}

std::string PBRMaterial::FindTextureFile(const std::string& directory, const std::vector<std::string>& possibleNames)
{
    for (const std::string& name : possibleNames) {
        std::string fullPath = directory + "/" + name;
        if (std::filesystem::exists(fullPath)) {
            return fullPath;
        }
    }
    return "";
}