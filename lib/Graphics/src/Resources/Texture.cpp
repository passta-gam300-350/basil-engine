#include <Resources/Texture.h>
#include <spdlog/spdlog.h>
#include <glad/glad.h>

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

    //stbi_set_flip_vertically_on_load(true);
    
    textureData.pixels = stbi_load(filename.c_str(), &textureData.width, 
                                  &textureData.height, &textureData.channels, 0);
    
    if (textureData.pixels) {
        textureData.isValid = true;
    } else {
        spdlog::error("Texture failed to load at path: {}", filename);
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
    
    GLenum internalFormat, dataFormat;
    if (data.channels == 1) {
        internalFormat = GL_RED;
        dataFormat = GL_RED;
    }
    else if (data.channels == 3) {
        // Use sRGB format for gamma-encoded textures (albedo/diffuse)
    	internalFormat = gamma ? GL_SRGB8 : GL_RGB;
        dataFormat = GL_RGB;
    }
	else if (data.channels == 4) {
        // Use sRGB format for gamma-encoded textures (albedo/diffuse with alpha)
        internalFormat = gamma ? GL_SRGB8_ALPHA8 : GL_RGBA;
        dataFormat = GL_RGBA;
    }
    else {
        internalFormat = GL_RGB; // Default fallback
    	dataFormat = GL_RGB;
    }
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, data.width, data.height, 0, dataFormat, GL_UNSIGNED_BYTE, data.pixels);
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

CubemapTextureData::CubemapTextureData(CubemapTextureData &&other) noexcept
    : faces(std::move(other.faces)), isValid(other.isValid)
{
    other.isValid = false;
}

CubemapTextureData &CubemapTextureData::operator=(CubemapTextureData &&other) noexcept
{
    if (this != &other)
    {
        faces = std::move(other.faces);
        isValid = other.isValid;
        other.isValid = false;
    }
    return *this;
}

CubemapTextureData TextureLoader::LoadCubemapFromFiles(
    const std::array<std::string, 6> &facePaths,
    const std::string &directory)
{
    CubemapTextureData cubemapData;

    // Face Order: +X, -X, +Y, -Y, +Z, -Z
    for (int i = 0; i < 6; ++i)
    {
        cubemapData.faces[i] = LoadFromFile(facePaths[i].c_str(), directory);
        if (!cubemapData.faces[i].isValid)
        {
            spdlog::error("Failed to load cubemap face {}: {}", i, facePaths[i]);
            cubemapData.isValid = false;
            return cubemapData;
        }
    }

    cubemapData.isValid = true;
    return cubemapData;
}


unsigned int TextureLoader::CreateGPUCubemap(const CubemapTextureData &data, bool generateMipmaps)
{
    if (!data.isValid)
    {
        spdlog::error("Invalid cubemap data provided");
        return 0;
    }

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    //Load all 6 faces
    for (int i = 0; i < 6; ++i)
    {
        const auto &face = data.faces[i];

        if (!face.isValid || !face.pixels)
        {
            spdlog::error("Invalid face data for cubemap face {}", i);
            glDeleteTextures(1, &textureID);
            return 0;
        }

        GLenum internalFormat, dataFormat;
        if (face.channels == 1)
        {
            internalFormat = GL_RED;
            dataFormat = GL_RED;
        }
        else if (face.channels == 3)
        {
            internalFormat = GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (face.channels == 4)
        {
            internalFormat = GL_RGBA;
            dataFormat = GL_RGBA;
        }
        else
        {
            internalFormat = GL_RGB;
            dataFormat = GL_RGB;
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat,
            face.width, face.height, 0, dataFormat, GL_UNSIGNED_BYTE, face.pixels);

    }

    // Set cubemap parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    if (generateMipmaps)
    {
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return textureID;
}


unsigned int TextureLoader::CubemapFromFiles(
    const std::array<std::string, 6> &facePaths,
    const std::string &directory,
    bool generateMipmaps)
{
    auto cubemapData = LoadCubemapFromFiles(facePaths, directory);
    return CreateGPUCubemap(cubemapData, generateMipmaps);
}