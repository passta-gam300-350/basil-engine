#include "Resources/BindlessTextureManager.h"

BindlessTextureManager::BindlessTextureManager() = default;

void BindlessTextureManager::BindTextures(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader) {
    if (!shader || !IsSupported()) {
        return;
    }

    // Process textures and generate handle data
    ProcessTextures(textures);

    // Update and bind SSBO immediately (no batching)
    m_SSBO.UpdateData(m_PendingHandles);
    m_SSBO.Bind();

    // Set shader uniforms for this specific object
    SetShaderUniforms(textures, shader);
}

void BindlessTextureManager::BeginBatch() {
    m_BatchActive = true;
    m_PendingHandles.clear();
}

void BindlessTextureManager::EndBatch() {
    if (m_BatchActive) {
        // Upload all pending handles at once
        m_SSBO.UpdateData(m_PendingHandles);
        m_SSBO.Bind();
        m_BatchActive = false;
    }
}

bool BindlessTextureManager::IsSupported() {
    return TextureHandleCache::IsBindlessSupported();
}

void BindlessTextureManager::ProcessTextures(const std::vector<Texture>& textures) {
    if (!m_BatchActive) {
        m_PendingHandles.clear();
    }

    for (const auto& texture : textures) {
        GLuint64 handle = m_HandleCache.GetHandle(texture.id);

        if (handle != 0) {
            TextureHandleData handleData;
            handleData.handle = handle;
            handleData.type = GetTextureTypeIndex(texture.type);
            handleData.flags = 1; // Mark as valid

            m_PendingHandles.push_back(handleData);
        }
    }
}

void BindlessTextureManager::SetShaderUniforms(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader) {
    // Set texture availability flags
    SetTextureAvailabilityFlags(textures, shader);

    // Set texture indices for shader access
    SetTextureIndices(textures, shader);
}

void BindlessTextureManager::SetTextureAvailabilityFlags(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader) {
    bool hasDiffuse = false, hasNormal = false, hasMetallic = false,
         hasRoughness = false, hasAO = false, hasEmissive = false,
         hasSpecular = false, hasHeight = false;

    for (const auto& texture : textures) {
        if (texture.type == "texture_diffuse") hasDiffuse = true;
        else if (texture.type == "texture_normal") hasNormal = true;
        else if (texture.type == "texture_metallic") hasMetallic = true;
        else if (texture.type == "texture_roughness") hasRoughness = true;
        else if (texture.type == "texture_ao") hasAO = true;
        else if (texture.type == "texture_emissive") hasEmissive = true;
        else if (texture.type == "texture_specular") hasSpecular = true;
        else if (texture.type == "texture_height") hasHeight = true;
    }

    shader->setBool("u_HasDiffuseMap", hasDiffuse);
    shader->setBool("u_HasNormalMap", hasNormal);
    shader->setBool("u_HasMetallicMap", hasMetallic);
    shader->setBool("u_HasRoughnessMap", hasRoughness);
    shader->setBool("u_HasAOMap", hasAO);
    shader->setBool("u_HasEmissiveMap", hasEmissive);
    shader->setBool("u_HasSpecularMap", hasSpecular);
    shader->setBool("u_HasHeightMap", hasHeight);
}

void BindlessTextureManager::SetTextureIndices(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader) {
    std::vector<int> textureIndices(8, -1); // Support up to 8 texture types

    for (size_t i = 0; i < textures.size() && i < TextureSSBO::MAX_TEXTURES; ++i) {
        uint32_t typeIndex = GetTextureTypeIndex(textures[i].type);
        if (typeIndex < 8) {
            textureIndices[typeIndex] = static_cast<int>(i);
        }
    }


    // Set texture index uniforms
    shader->setInt("u_DiffuseIndex", textureIndices[0]);
    shader->setInt("u_NormalIndex", textureIndices[1]);
    shader->setInt("u_MetallicIndex", textureIndices[2]);
    shader->setInt("u_RoughnessIndex", textureIndices[3]);
    shader->setInt("u_AOIndex", textureIndices[4]);
    shader->setInt("u_EmissiveIndex", textureIndices[5]);
    shader->setInt("u_SpecularIndex", textureIndices[6]);
    shader->setInt("u_HeightIndex", textureIndices[7]);
}

uint32_t BindlessTextureManager::GetTextureTypeIndex(const std::string& type) const {
    if (type == "texture_diffuse") return 0;
    if (type == "texture_normal") return 1;
    if (type == "texture_metallic") return 2;
    if (type == "texture_roughness") return 3;
    if (type == "texture_ao") return 4;
    if (type == "texture_emissive") return 5;
    if (type == "texture_specular") return 6;
    if (type == "texture_height") return 7;
    return 0; // Default to diffuse
}