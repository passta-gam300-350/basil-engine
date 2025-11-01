/******************************************************************************/
/*!
\file   TextureSlotManager.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the TextureSlotManager class for managing texture binding slots in OpenGL

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "Resources/Texture.h"
#include "Resources/Shader.h"
#include <vector>
#include <memory>
#include <string>
#include <array>

// Texture slot definitions - using #define directives for clear mapping
#define TEXTURE_SLOT_DIFFUSE    0
#define TEXTURE_SLOT_NORMAL     1
#define TEXTURE_SLOT_METALLIC   2
#define TEXTURE_SLOT_ROUGHNESS  3
#define TEXTURE_SLOT_AO         4
#define TEXTURE_SLOT_EMISSIVE   5
#define TEXTURE_SLOT_SPECULAR   6
#define TEXTURE_SLOT_HEIGHT     7
#define TEXTURE_SLOT_SHADOW     8

// Maximum number of texture slots available
#define MAX_TEXTURE_SLOTS       16

class TextureSlotManager {
public:
    TextureSlotManager();
    ~TextureSlotManager() = default;

    // Main interface - bind textures using traditional OpenGL texture units
    void BindTextures(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader);

    // Bind a single texture to a specific slot
    void BindTexture(const Texture& texture, int slot);

    // Bind a texture by OpenGL texture ID to a specific slot
    void BindTextureID(unsigned int textureID, int slot);

    // Unbind texture from specific slot
    void UnbindTexture(int slot);

    // Unbind all textures
    void UnbindAll();

    // Get the texture slot for a given texture type string
    static int GetTextureSlot(const std::string& textureType);

    // Get the uniform name for a texture slot
    static std::string GetTextureUniformName(int slot);

    // Get texture type name from slot
    static std::string GetTextureTypeName(int slot);

    // Check if a slot is currently bound
    bool IsSlotBound(int slot) const;

    // Get currently bound texture ID for a slot
    unsigned int GetBoundTexture(int slot) const;

private:
    // Track currently bound textures per slot
    std::array<unsigned int, MAX_TEXTURE_SLOTS> m_BoundTextures;
    std::array<bool, MAX_TEXTURE_SLOTS> m_SlotUsed;

    // Helper methods
    static void SetShaderUniforms(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader);
    static void SetTextureAvailabilityFlags(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader);
    static void SetTextureUniformSamplers(const std::shared_ptr<Shader>& shader);

    // Internal binding implementation
    void BindTextureToSlot(unsigned int textureID, int slot);
};