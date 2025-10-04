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

// GPU texture handle (separate from CPU data)
struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
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
};

