/******************************************************************************/
/*!
\file   Texture.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares texture data structures and loading utilities for CPU and GPU texture management

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <string>
#include <tinyddsloader.h>
#include <array>
#include <glad/glad.h>

// CPU-side texture data (decoupled from GPU)
struct TextureData {
    unsigned char* pixels;
    int width;
    int height;
    int channels;
    bool isValid;
    
    TextureData() : pixels(nullptr), width(0), height(0), channels(0), isValid(false) {}
    ~TextureData();
    
    // Non-copyable but movable
    TextureData(const TextureData&) = delete;
    TextureData& operator=(const TextureData&) = delete;
    TextureData(TextureData&& other) noexcept;
    TextureData& operator=(TextureData&& other) noexcept;
};

struct CubemapTextureData
{
    std::array<TextureData, 6> faces;
    bool isValid = true;

    CubemapTextureData() = default;
    ~CubemapTextureData() = default;

    // Non-copyable but movable
    CubemapTextureData(const CubemapTextureData &) = delete;
    CubemapTextureData &operator=(const CubemapTextureData &) = delete;
    CubemapTextureData(CubemapTextureData &&other) noexcept;
    CubemapTextureData &operator=(CubemapTextureData &&other) noexcept;
};

// GPU texture handle (separate from CPU data)
struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
    GLenum target = GL_TEXTURE_2D;
};

// Decoupled resource management
class TextureLoader {
public:
    // CPU-only loading (no OpenGL calls)
    static TextureData LoadFromFile(const char* path, const std::string& directory);
    
    // GPU resource creation (separated concern)
    static unsigned int CreateGPUTexture(const TextureData& data, bool gamma = false);

    // GPU resource creation (separated concern). loads compressed bc1 bc3 bc4 bc5 files. bc7 not supported
    static unsigned int CreateGPUTextureCompressed(tinyddsloader::DDSFile& ddsimg);
    
    // Legacy function for backward compatibility
    static unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma = false);

    static CubemapTextureData LoadCubemapFromFiles(
        const std::array<std::string, 6> &facePaths,
        const std::string &directory
    );

    static unsigned int CreateGPUCubemap(const CubemapTextureData &data, bool generateMipmaps = true);

    static unsigned int CubemapFromFiles(
        const std::array<std::string, 6> &facePaths,
        const std::string &directory,
        bool generateMipmaps = true
    );

    /**
     * @brief Create a cubemap from 6 existing 2D texture IDs (for resource pipeline integration)
     * @param textureIDs Array of 6 OpenGL texture IDs in order: +X, -X, +Y, -Y, +Z, -Z
     * @param generateMipmaps Whether to generate mipmaps for the cubemap
     * @return OpenGL cubemap texture ID, or 0 on failure
     * @note This copies pixel data from existing textures to a new cubemap
     */
    static unsigned int CubemapFromTextureIDs(
        const std::array<unsigned int, 6>& textureIDs,
        bool generateMipmaps = true
    );
};

