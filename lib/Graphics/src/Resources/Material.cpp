#include <Resources/Material.h>
#include <iostream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

Material::Material(std::shared_ptr<Shader> const &shader, std::string const &name)
	: m_Shader(shader), m_Name(name), m_NextTextureUnit(0)
{
	if (!shader)
	{
		std::cerr << "Warning: Material '" << name << "' created with null shader!" << std::endl;
	}

    // Setup texture type to uniform mappings (matching basic.frag shader)
    m_TextureTypeToUniform[Texture::Type::Diffuse] = "texture_diffuse1";
    m_TextureTypeToUniform[Texture::Type::BaseColor] = "texture_diffuse1";
    m_TextureTypeToUniform[Texture::Type::Normal] = "texture_normal1";
    m_TextureTypeToUniform[Texture::Type::Metallic] = "texture_metallic1";
    m_TextureTypeToUniform[Texture::Type::Roughness] = "texture_roughness1";
    m_TextureTypeToUniform[Texture::Type::AmbientOcclusion] = "texture_ao1";
    m_TextureTypeToUniform[Texture::Type::Emissive] = "texture_emissive1";
    m_TextureTypeToUniform[Texture::Type::Height] = "texture_height1";
    m_TextureTypeToUniform[Texture::Type::Specular] = "texture_specular1";
}

void Material::Bind()
{
    if (!m_Shader)
    {
        std::cerr << "Cannot bind material '" << m_Name << "' with null shader!" << std::endl;
        return;
    }

    // Bind the shader
    m_Shader->Bind();

    // Bind all textures and set their uniform values
    for (const auto &[uniformName, textureInfo] : m_Textures)
    {
        if (textureInfo.texture)
        {
            // Bind texture to its assigned slot
            textureInfo.texture->Bind(textureInfo.textureUnit);

            // Set the uniform to point to the correct texture slot
            m_Shader->SetInt(uniformName, static_cast<int>(textureInfo.textureUnit));
        }
    }

}

void Material::Unbind()
{
    if (m_Shader)
    {
        m_Shader->Unbind();
    }

    // Note: We don't explicitly unbind textures as they'll be overridden by the next material
}

void Material::SetFloat(std::string const &name, float value)
{
    if (m_Shader)
    {
        m_Shader->SetFloat(name, value);
        m_FloatProperties[name] = value; // Cache for debugging
    }
}

void Material::SetInt(std::string const &name, int value)
{
    if (m_Shader)
    {
        m_Shader->SetInt(name, value);
    }
}

void Material::SetBool(std::string const &name, bool value)
{
    if (m_Shader)
    {
        m_Shader->SetInt(name, value ? 1 : 0);
    }
}

void Material::SetVec2(std::string const &name, glm::vec2 const &value)
{
    if (m_Shader)
    {
        m_Shader->SetFloat2(name, value);
    }
}

void Material::SetVec3(std::string const &name, glm::vec3 const &value)
{
    if (m_Shader)
    {
        m_Shader->SetFloat3(name, value);
        m_Vec3Properties[name] = value; // Cache for debugging
    }
}

void Material::SetVec4(std::string const &name, glm::vec4 const &value)
{
    if (m_Shader)
    {
        m_Shader->SetFloat4(name, value);
        m_Vec4Properties[name] = value; // Cache for debugging
    }
}

void Material::SetMat3(std::string const &name, glm::mat3 const &value)
{
    if (m_Shader)
    {
        m_Shader->SetMat3(name, value);
    }
}

void Material::SetMat4(std::string const &name, glm::mat4 const &value)
{
    if (m_Shader)
    {
        m_Shader->SetMat4(name, value);
    }
}

void Material::SetTexture(std::string const &uniformName, std::shared_ptr<Texture> const &texture)
{
    if (!texture)
    {
        RemoveTexture(uniformName);
        return;
    }

    // Check if uniform has the texture
    auto it = m_Textures.find(uniformName);
    if (it != m_Textures.end())
    {
        // Update existing texture, keep the same texture unit
        it->second.texture = texture;
    }
    else
    {
        // Assign new texture to new texture unit
        TextureInfo info;
        info.texture = texture;
        info.textureUnit = m_NextTextureUnit++;

        m_Textures[uniformName] = info;

        if (m_NextTextureUnit > 32)
        { // We assume that the GPUs support at least 32 texture units
            std::cerr << "Warning: Material '" << m_Name << "' using more than 32 texture units!" << std::endl;
        }

    }

    std::cout << "Material '" << m_Name << "': Set texture " << uniformName
        << " (type: " << Texture::TypeToString(texture->GetType()) << ")"
        << " to slot " << m_Textures[uniformName].textureUnit << std::endl;
}

void Material::SetTexture(Texture::Type type, std::shared_ptr<Texture> const &texture)
{
    std::string uniformName = GetUniformNameForTextureType(type);
    if (uniformName.empty())
    {
        std::cerr << "Warning: No uniform mapping for texture type: "
            << Texture::TypeToString(type) << std::endl;
        return;
    }

    SetTexture(uniformName, texture);
}

void Material::RemoveTexture(std::string const &uniformName)
{
    auto it = m_Textures.find(uniformName);
    if (it != m_Textures.end())
    {
        std::cout << "Material '" << m_Name << "': Removed texture " << uniformName << std::endl;
        m_Textures.erase(it);

        // Note: We don't reuse texture units to avoid confusion during a frame
        // Unit optimization could be added later if needed
    }
}

void Material::RemoveTexture(Texture::Type type)
{
    std::string uniformName = GetUniformNameForTextureType(type);
    if (!uniformName.empty())
    {
        RemoveTexture(uniformName);
    }
}

bool Material::HasTexture(std::string const &uniformName) const
{
    return m_Textures.find(uniformName) != m_Textures.end();
}

bool Material::HasTexture(Texture::Type type) const
{
    std::string uniformName = GetUniformNameForTextureType(type);
    return !uniformName.empty() && HasTexture(uniformName);
}

std::shared_ptr<Texture> Material::GetTexture(std::string const &uniformName) const
{
    auto it = m_Textures.find(uniformName);
    return (it != m_Textures.end()) ? it->second.texture : nullptr;
}

std::shared_ptr<Texture> Material::GetTexture(Texture::Type type) const
{
    std::string uniformName = GetUniformNameForTextureType(type);
    return !uniformName.empty() ? GetTexture(uniformName) : nullptr;
}

std::unordered_map<std::string, std::shared_ptr<Texture>> Material::GetTextures() const
{
    std::unordered_map<std::string, std::shared_ptr<Texture>> result;
    for (const auto &[name, textureInfo] : m_Textures)
    {
        result[name] = textureInfo.texture;
    }
    return result;
}

void Material::LoadFromAssimp(aiMaterial *assimpMaterial, std::string const &directory)
{
    if (!assimpMaterial)
    {
        std::cerr << "Error: Null aiMaterial passed to LoadFromAssimp" << std::endl;
        return;
    }

    std::cout << "Loading material properties from Assimp" << std::endl;

    // Load material properties
    aiColor3D color;
    float value;

    // Diffuse/Albedo color
    if (assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
    {
        SetAlbedo(glm::vec3(color.r, color.g, color.b));
        std::cout << "  Albedo: (" << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
    }

    // Specular color (not used in basic shader, but keep for compatibility)
    if (assimpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
    {
        // The basic shader doesn't use this uniform, but we can store it
        // SetVec3("u_Material.specular", glm::vec3(color.r, color.g, color.b));
    }

    // Emissive color
    if (assimpMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS)
    {
        SetEmission(glm::vec3(color.r, color.g, color.b));
    }

    // Shininess (convert to roughness: roughness = sqrt(2.0 / (shininess + 2.0)))
    if (assimpMaterial->Get(AI_MATKEY_SHININESS, value) == AI_SUCCESS)
    {
        float roughness = sqrt(2.0f / (value + 2.0f));
        SetRoughness(roughness);
        std::cout << "  Shininess: " << value << " -> Roughness: " << roughness << std::endl;
    }

    // Metallic factor (if available)
    if (assimpMaterial->Get(AI_MATKEY_METALLIC_FACTOR, value) == AI_SUCCESS)
    {
        SetMetallic(value);
        std::cout << "  Metallic: " << value << std::endl;
    }

    // Roughness factor (if available)
    if (assimpMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, value) == AI_SUCCESS)
    {
        SetRoughness(value);
        std::cout << "  Roughness: " << value << std::endl;
    }

    // Opacity (not used in basic shader directly)
    if (assimpMaterial->Get(AI_MATKEY_OPACITY, value) == AI_SUCCESS)
    {
        // The basic shader doesn't use this uniform
        // SetFloat("u_Material.opacity", value);
    }

    // Temporarily disable texture loading from Assimp to avoid crashes
    std::cout << "Skipping Assimp texture loading (will use manually loaded textures instead)" << std::endl;
    
    // TODO: Fix Texture constructor crash and re-enable this
    /*
    // Load textures for all types with error handling
    try {
        LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_DIFFUSE);
        LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_SPECULAR);
        LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_NORMALS);
        LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_HEIGHT);
        LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_EMISSIVE);
        LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_METALNESS);
        LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_DIFFUSE_ROUGHNESS);
        LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_AMBIENT_OCCLUSION);
        LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_BASE_COLOR);
    } catch (const std::exception& e) {
        std::cerr << "Exception during texture loading from Assimp: " << e.what() << std::endl;
    }
    */
}

Texture::Type Material::AssimpTextureTypeToEngineType(aiTextureType assimpType)
{
    switch (assimpType)
    {
        case aiTextureType_DIFFUSE:           return Texture::Type::Diffuse;
        case aiTextureType_SPECULAR:          return Texture::Type::Specular;
        case aiTextureType_NORMALS:           return Texture::Type::Normal;
        case aiTextureType_HEIGHT:            return Texture::Type::Height;
        case aiTextureType_EMISSIVE:          return Texture::Type::Emissive;
        case aiTextureType_METALNESS:         return Texture::Type::Metallic;
        case aiTextureType_DIFFUSE_ROUGHNESS: return Texture::Type::Roughness;
        case aiTextureType_AMBIENT_OCCLUSION: return Texture::Type::AmbientOcclusion;
        case aiTextureType_BASE_COLOR:        return Texture::Type::BaseColor;
        case aiTextureType_AMBIENT:           return Texture::Type::Ambient;
        case aiTextureType_SHININESS:         return Texture::Type::Shininess;
        case aiTextureType_OPACITY:           return Texture::Type::Opacity;
        case aiTextureType_DISPLACEMENT:      return Texture::Type::Displacement;
        case aiTextureType_LIGHTMAP:          return Texture::Type::Lightmap;
        case aiTextureType_REFLECTION:        return Texture::Type::Reflection;
        default:                              return Texture::Type::Unknown;
    }
}

void Material::UpdateTextureUnits()
{
    if (!m_Shader) return;

    m_Shader->Bind();
    for (const auto &[uniformName, textureInfo] : m_Textures)
    {
        m_Shader->SetInt(uniformName, static_cast<int>(textureInfo.textureUnit));
    }
}

std::string Material::GetUniformNameForTextureType(Texture::Type type) const
{
    auto it = m_TextureTypeToUniform.find(type);
    return (it != m_TextureTypeToUniform.end()) ? it->second : "";
}

void Material::LoadTexturesFromAssimp(aiMaterial *assimpMaterial, std::string const &directory, aiTextureType textureType)
{
    if (!assimpMaterial) {
        std::cerr << "Error: Null aiMaterial in LoadTexturesFromAssimp" << std::endl;
        return;
    }

    unsigned int textureCount = 0;
    try {
        textureCount = assimpMaterial->GetTextureCount(textureType);
    } catch (const std::exception& e) {
        std::cerr << "Exception getting texture count: " << e.what() << std::endl;
        return;
    }

    for (unsigned int i = 0; i < textureCount; i++)
    {
        aiString texturePath;
        if (assimpMaterial->GetTexture(textureType, i, &texturePath) == AI_SUCCESS)
        {
            std::string texturePathStr = std::string(texturePath.C_Str());
            std::string fullPath;
            
            std::cout << "  Found texture reference: " << texturePathStr << std::endl;
            std::cout << "  Directory context: " << directory << std::endl;
            
            // Check if the texture path is already absolute
            if (fs::path(texturePathStr).is_absolute())
            {
                fullPath = texturePathStr;
                std::cout << "  Using absolute path: " << fullPath << std::endl;
            }
            else
            {
                // For relative paths, first try with absolute asset base path
                std::string assetBase = "D:/Projects/CSD3401/test/examples/lib/Graphics/assets/models/tinbox/";
                fullPath = assetBase + texturePathStr;
                std::cout << "  Trying absolute path: " << fullPath << std::endl;
                
                // If not found with absolute path, try the relative directory path as fallback
                if (!fs::exists(fullPath))
                {
                    fullPath = directory + "/" + texturePathStr;
                    std::cout << "  Trying relative path: " << fullPath << std::endl;
                }
            }

            // Check if file exists
            if (!fs::exists(fullPath))
            {
                std::cerr << "Warning: Texture file not found: " << fullPath << std::endl;
                std::cerr << "  Original path: " << texturePathStr << std::endl;
                std::cerr << "  Directory: " << directory << std::endl;
                continue;
            }

            // Convert Assimp texture type to engine type
            Texture::Type engineType = AssimpTextureTypeToEngineType(textureType);

            // Create texture with appropriate properties
            Texture::Properties props;
            props.type = engineType;
            props.generateMipmaps = true;

            // Special handling for normal maps
            if (textureType == aiTextureType_NORMALS || textureType == aiTextureType_HEIGHT)
            {
                props.flipOnLoad = false; // Normal maps usually shouldn't be flipped
            }

            std::cout << "  Attempting to load texture: " << fullPath << std::endl;
            
            // First verify the file actually exists and is readable
            if (!fs::exists(fullPath)) {
                std::cerr << "  ✗ File does not exist: " << fullPath << std::endl;
                continue;
            }
            
            if (!fs::is_regular_file(fullPath)) {
                std::cerr << "  ✗ Not a regular file: " << fullPath << std::endl;
                continue;
            }
            
            std::cout << "  File exists, creating Texture object..." << std::endl;
            
            try
            {
                std::cout << "  Calling Texture constructor..." << std::endl;
                auto texture = std::make_shared<Texture>(fullPath, props);
                std::cout << "  Texture constructor completed, checking if loaded..." << std::endl;
                
                if (texture && texture->IsLoaded())
                {
                    std::cout << "  Setting texture on material..." << std::endl;
                    SetTexture(engineType, texture);
                    std::cout << "  ✓ Loaded " << Texture::TypeToString(engineType)
                        << " texture: " << texturePathStr << std::endl;
                }
                else
                {
                    std::cerr << "  ✗ Failed to load texture: " << fullPath << std::endl;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "  ✗ Exception loading texture " << fullPath << ": " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "  ✗ Unknown exception loading texture " << fullPath << std::endl;
            }
        }
    }
}