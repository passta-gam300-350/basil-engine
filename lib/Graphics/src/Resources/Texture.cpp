#include <Resources/Texture.h>
#include <spdlog/spdlog.h>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// TextureData implementation
TextureData::~TextureData() {
    if (pixels != nullptr) {
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
        if (pixels != nullptr) {
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

    //stbi_set_flip_vertically_on_load(true);
    
    textureData.pixels = stbi_load(filename.c_str(), &textureData.width,
                                  &textureData.height, &textureData.channels, 0);

    if (textureData.pixels != nullptr) {
        textureData.isValid = true;
    } else {
        spdlog::error("Texture failed to load at path: {}", filename);
        textureData.isValid = false;
    }
    
    return textureData;
}

// GPU resource creation (separated concern)
unsigned int TextureLoader::CreateGPUTexture(const TextureData& data, bool gamma) {
    if (!data.isValid || data.pixels == nullptr) {
        return 0;
    }

    unsigned int textureID = 0;
    glGenTextures(1, &textureID);

    GLenum internalFormat = GL_RGB;
    GLenum dataFormat = GL_RGB;
    if (data.channels == 1) {
        internalFormat = GL_RED;
        dataFormat = GL_RED;
    } else if (data.channels == 3) {
        // Use sRGB format for gamma-encoded textures (albedo/diffuse)
        internalFormat = gamma ? GL_SRGB8 : GL_RGB;
        dataFormat = GL_RGB;
    } else if (data.channels == 4) {
        // Use sRGB format for gamma-encoded textures (albedo/diffuse with alpha)
        internalFormat = gamma ? GL_SRGB8_ALPHA8 : GL_RGBA;
        dataFormat = GL_RGBA;
    } else {
        internalFormat = GL_RGB; // Default fallback
        dataFormat = GL_RGB;
    }
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat), data.width, data.height, 0, dataFormat, GL_UNSIGNED_BYTE, data.pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    return textureID;
}

unsigned int TextureLoader::CreateGPUTextureCompressed(tinyddsloader::DDSFile& ddsimg) {
    const auto& img = ddsimg.GetImageData(0, 0);

    GLenum glCompressedFormat = 0;
    
    // map block compression format, bc7 not support. too bad. its too expensive anyways
    switch (ddsimg.GetFormat()) {
    case tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm:  glCompressedFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; break;
    case tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm:  glCompressedFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
    case tinyddsloader::DDSFile::DXGIFormat::BC4_UNorm:  glCompressedFormat = GL_COMPRESSED_RED_RGTC1; break;
    case tinyddsloader::DDSFile::DXGIFormat::BC5_UNorm:  glCompressedFormat = GL_COMPRESSED_RG_RGTC2; break;
    default: return 0;
    }

    unsigned int textureID{};
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glCompressedTexImage2D(GL_TEXTURE_2D, 0, glCompressedFormat,
        static_cast<GLsizei>(ddsimg.GetWidth()),
        static_cast<GLsizei>(ddsimg.GetHeight()),
        0,
        static_cast<GLsizei>(ddsimg.GetImageData(0,0)->m_memSlicePitch),
        ddsimg.GetImageData(0,0)->m_mem);
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

