#include "../../include/Resources/TextureSlotManager.h"
#include <glad/glad.h>
#include <spdlog/spdlog.h>
#include <algorithm>

TextureSlotManager::TextureSlotManager() {
    // Initialize all slots as unbound
    m_BoundTextures.fill(0);
    m_SlotUsed.fill(false);
}

void TextureSlotManager::BindTextures(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader) {
    if (!shader) {
        return;
    }

    // Clear material texture slots (0-7)
    for (int i = 0; i < 8; ++i) {
        UnbindTexture(i);
    }

    // Bind each texture to its appropriate slot
    for (const auto& texture : textures) {
        int slot = GetTextureSlot(texture.type);
        if (slot >= 0 && slot < MAX_TEXTURE_SLOTS) {
            BindTexture(texture, slot);
        } else {
            spdlog::warn("[TextureSlotManager] Unknown texture type: {}", texture.type);
        }
    }

    // Set shader uniforms
    SetShaderUniforms(textures, shader);
}

void TextureSlotManager::BindTexture(const Texture& texture, int slot) {
    if (slot < 0 || slot >= MAX_TEXTURE_SLOTS) {
        spdlog::error("[TextureSlotManager] Invalid texture slot: {}", slot);
        return;
    }

    BindTextureToSlot(texture.id, slot);
}

void TextureSlotManager::BindTextureID(unsigned int textureID, int slot) {
    if (slot < 0 || slot >= MAX_TEXTURE_SLOTS) {
        spdlog::error("[TextureSlotManager] Invalid texture slot: {}", slot);
        return;
    }

    BindTextureToSlot(textureID, slot);
}

void TextureSlotManager::BindTextureToSlot(unsigned int textureID, int slot) {
    // Activate the texture unit
    glActiveTexture(GL_TEXTURE0 + slot);

    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Track the binding
    m_BoundTextures[slot] = textureID;
    m_SlotUsed[slot] = (textureID != 0);
}

void TextureSlotManager::UnbindTexture(int slot) {
    if (slot < 0 || slot >= MAX_TEXTURE_SLOTS) {
        return;
    }

    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_BoundTextures[slot] = 0;
    m_SlotUsed[slot] = false;
}

void TextureSlotManager::UnbindAll() {
    for (int i = 0; i < MAX_TEXTURE_SLOTS; ++i) {
        if (m_SlotUsed[i]) {
            UnbindTexture(i);
        }
    }
}

int TextureSlotManager::GetTextureSlot(const std::string& textureType) {
    if (textureType == "texture_diffuse") return TEXTURE_SLOT_DIFFUSE;
    if (textureType == "texture_normal") return TEXTURE_SLOT_NORMAL;
    if (textureType == "texture_metallic") return TEXTURE_SLOT_METALLIC;
    if (textureType == "texture_roughness") return TEXTURE_SLOT_ROUGHNESS;
    if (textureType == "texture_ao") return TEXTURE_SLOT_AO;
    if (textureType == "texture_emissive") return TEXTURE_SLOT_EMISSIVE;
    if (textureType == "texture_specular") return TEXTURE_SLOT_SPECULAR;
    if (textureType == "texture_height") return TEXTURE_SLOT_HEIGHT;
    return -1; // Unknown type
}

std::string TextureSlotManager::GetTextureUniformName(int slot) {
    switch (slot) {
        case TEXTURE_SLOT_DIFFUSE:   return "u_DiffuseMap";
        case TEXTURE_SLOT_NORMAL:    return "u_NormalMap";
        case TEXTURE_SLOT_METALLIC:  return "u_MetallicMap";
        case TEXTURE_SLOT_ROUGHNESS: return "u_RoughnessMap";
        case TEXTURE_SLOT_AO:        return "u_AOMap";
        case TEXTURE_SLOT_EMISSIVE:  return "u_EmissiveMap";
        case TEXTURE_SLOT_SPECULAR:  return "u_SpecularMap";
        case TEXTURE_SLOT_HEIGHT:    return "u_HeightMap";
        default:                     return "";
    }
}

std::string TextureSlotManager::GetTextureTypeName(int slot) {
    switch (slot) {
        case TEXTURE_SLOT_DIFFUSE:   return "texture_diffuse";
        case TEXTURE_SLOT_NORMAL:    return "texture_normal";
        case TEXTURE_SLOT_METALLIC:  return "texture_metallic";
        case TEXTURE_SLOT_ROUGHNESS: return "texture_roughness";
        case TEXTURE_SLOT_AO:        return "texture_ao";
        case TEXTURE_SLOT_EMISSIVE:  return "texture_emissive";
        case TEXTURE_SLOT_SPECULAR:  return "texture_specular";
        case TEXTURE_SLOT_HEIGHT:    return "texture_height";
        default:                     return "";
    }
}

bool TextureSlotManager::IsSlotBound(int slot) const {
    if (slot < 0 || slot >= MAX_TEXTURE_SLOTS) {
        return false;
    }
    return m_SlotUsed[slot];
}

unsigned int TextureSlotManager::GetBoundTexture(int slot) const {
    if (slot < 0 || slot >= MAX_TEXTURE_SLOTS) {
        return 0;
    }
    return m_BoundTextures[slot];
}

void TextureSlotManager::SetShaderUniforms(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader) {
    // Set texture availability flags
    SetTextureAvailabilityFlags(textures, shader);

    // Set texture uniform samplers
    SetTextureUniformSamplers(shader);
}

void TextureSlotManager::SetTextureAvailabilityFlags(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader) {
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

void TextureSlotManager::SetTextureUniformSamplers(const std::shared_ptr<Shader>& shader) {
    // Set sampler uniforms to point to the correct texture units
    shader->setInt("u_DiffuseMap", TEXTURE_SLOT_DIFFUSE);
    shader->setInt("u_NormalMap", TEXTURE_SLOT_NORMAL);
    shader->setInt("u_MetallicMap", TEXTURE_SLOT_METALLIC);
    shader->setInt("u_RoughnessMap", TEXTURE_SLOT_ROUGHNESS);
    shader->setInt("u_AOMap", TEXTURE_SLOT_AO);
    shader->setInt("u_EmissiveMap", TEXTURE_SLOT_EMISSIVE);
    shader->setInt("u_SpecularMap", TEXTURE_SLOT_SPECULAR);
    shader->setInt("u_HeightMap", TEXTURE_SLOT_HEIGHT);
}