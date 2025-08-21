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

    // Setup default texture type to uniform mappings (PBR standard)
    m_TextureTypeToUniform[Texture::Type::Diffuse] = "u_Material.diffuse";
    m_TextureTypeToUniform[Texture::Type::BaseColor] = "u_Material.albedo";
    m_TextureTypeToUniform[Texture::Type::Normal] = "u_Material.normal";
    m_TextureTypeToUniform[Texture::Type::Metallic] = "u_Material.metallic";
    m_TextureTypeToUniform[Texture::Type::Roughness] = "u_Material.roughness";
    m_TextureTypeToUniform[Texture::Type::AmbientOcclusion] = "u_Material.ao";
    m_TextureTypeToUniform[Texture::Type::Emissive] = "u_Material.emission";
    m_TextureTypeToUniform[Texture::Type::Height] = "u_Material.height";
    m_TextureTypeToUniform[Texture::Type::Specular] = "u_Material.specular";
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

    // Specular color
    if (assimpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
    {
        SetVec3("u_Material.specular", glm::vec3(color.r, color.g, color.b));
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

    // Opacity
    if (assimpMaterial->Get(AI_MATKEY_OPACITY, value) == AI_SUCCESS)
    {
        SetFloat("u_Material.opacity", value);
    }

    // Load textures for all types
    LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_DIFFUSE);
    LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_SPECULAR);
    LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_NORMALS);
    LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_HEIGHT);
    LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_EMISSIVE);
    LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_METALNESS);
    LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_DIFFUSE_ROUGHNESS);
    LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_AMBIENT_OCCLUSION);
    LoadTexturesFromAssimp(assimpMaterial, directory, aiTextureType_BASE_COLOR);
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
    unsigned int textureCount = assimpMaterial->GetTextureCount(textureType);

    for (unsigned int i = 0; i < textureCount; i++)
    {
        aiString texturePath;
        if (assimpMaterial->GetTexture(textureType, i, &texturePath) == AI_SUCCESS)
        {
            std::string fullPath = directory + "/" + std::string(texturePath.C_Str());

            // Check if file exists
            if (!fs::exists(fullPath))
            {
                std::cerr << "Warning: Texture file not found: " << fullPath << std::endl;
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

            try
            {
                auto texture = std::make_shared<Texture>(fullPath, props);
                if (texture->IsLoaded())
                {
                    SetTexture(engineType, texture);
                    std::cout << "  Loaded " << Texture::TypeToString(engineType)
                        << " texture: " << texturePath.C_Str() << std::endl;
                }
                else
                {
                    std::cerr << "Failed to load texture: " << fullPath << std::endl;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Exception loading texture " << fullPath << ": " << e.what() << std::endl;
            }
        }
    }
}