#pragma once

#include <string>
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
};

