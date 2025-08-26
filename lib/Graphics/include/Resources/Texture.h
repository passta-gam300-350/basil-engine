#pragma once

#include <glad/gl.h>
#include <string>
#include <memory>

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
    
    // Legacy function for backward compatibility
    static unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma = false);
};

// Legacy function for backward compatibility
unsigned int TextureFromFile(const char *path, const std::string &directory, bool gamma = false);