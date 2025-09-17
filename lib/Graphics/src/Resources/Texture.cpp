#include <Resources/Texture.h>
#include <iostream>
#include <glad/gl.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// TextureData implementation
TextureData::~TextureData() {
    if (pixels) {
        stbi_image_free(pixels);
        pixels = nullptr;
    }
}

TextureData::TextureData(TextureData&& other) noexcept 
    : pixels(other.pixels), width(other.width), height(other.height), 
      channels(other.channels), isValid(other.isValid) {
    other.pixels = nullptr;
    other.width = 0;
    other.height = 0;
    other.channels = 0;
    other.isValid = false;
}

TextureData& TextureData::operator=(TextureData&& other) noexcept {
    if (this != &other) {
        if (pixels) {
            stbi_image_free(pixels);
        }
        pixels = other.pixels;
        width = other.width;
        height = other.height;
        channels = other.channels;
        isValid = other.isValid;
        
        other.pixels = nullptr;
        other.width = 0;
        other.height = 0;
        other.channels = 0;
        other.isValid = false;
    }
    return *this;
}

// CPU-only loading (decoupled from GPU)
TextureData TextureLoader::LoadFromFile(const char* path, const std::string& directory) {
    TextureData textureData;
    std::string filename = directory + '/' + std::string(path);
    
    textureData.pixels = stbi_load(filename.c_str(), &textureData.width, 
                                  &textureData.height, &textureData.channels, 0);
    
    if (textureData.pixels) {
        textureData.isValid = true;
    } else {
        std::cout << "Texture failed to load at path: " << filename << std::endl;
        textureData.isValid = false;
    }
    
    return textureData;
}

// GPU resource creation (separated concern)
unsigned int TextureLoader::CreateGPUTexture(const TextureData& data, bool gamma) {
    if (!data.isValid || !data.pixels) {
        return 0;
    }
    
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    GLenum format;
    if (data.channels == 1)
        format = GL_RED;
    else if (data.channels == 3)
        format = GL_RGB;
    else if (data.channels == 4)
        format = GL_RGBA;
    else
        format = GL_RGB; // Default fallback
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, data.width, data.height, 0, format, GL_UNSIGNED_BYTE, data.pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    return textureID;
}

// Legacy function (uses new decoupled approach internally)
unsigned int TextureLoader::TextureFromFile(const char* path, const std::string& directory, bool gamma) {
    auto textureData = LoadFromFile(path, directory);
    return CreateGPUTexture(textureData, gamma);
}

