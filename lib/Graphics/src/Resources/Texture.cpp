/******************************************************************************/
/*!
\file   Texture.cpp
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Implementation of texture loading and GPU resource management

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include <Resources/Texture.h>
#include <spdlog/spdlog.h>
#include <glad/glad.h>
#include <cmath>

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

    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat = GL_RGB;
    if (data.channels == 1) {
        internalFormat = GL_R8;  // DSA requires sized format
        dataFormat = GL_RED;
    } else if (data.channels == 3) {
        // Use sRGB format for gamma-encoded textures (albedo/diffuse)
        internalFormat = gamma ? GL_SRGB8 : GL_RGB8;  // DSA requires sized format
        dataFormat = GL_RGB;
    } else if (data.channels == 4) {
        // Use sRGB format for gamma-encoded textures (albedo/diffuse with alpha)
        internalFormat = gamma ? GL_SRGB8_ALPHA8 : GL_RGBA8;  // DSA requires sized format
        dataFormat = GL_RGBA;
    } else {
        internalFormat = GL_RGB8; // Default fallback (DSA requires sized format)
        dataFormat = GL_RGB;
    }

    // DSA: Create texture without binding
    glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

    // Calculate mipmap levels (1 + floor(log2(max(width, height))))
    GLsizei levels = 1 + static_cast<GLsizei>(floor(log2(std::max(data.width, data.height))));

    // DSA: Allocate immutable storage (preferred in modern GL)
    glTextureStorage2D(textureID, levels, internalFormat, data.width, data.height);

    // DSA: Upload pixel data to base level without binding
    glTextureSubImage2D(textureID, 0, 0, 0, data.width, data.height, dataFormat, GL_UNSIGNED_BYTE, data.pixels);

    // DSA: Generate mipmaps without binding
    glGenerateTextureMipmap(textureID);

    // Enable anisotropic filtering for maximum texture clarity (matches ogldev)
    GLfloat maxAnisotropy = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
    glTextureParameterf(textureID, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);

    // DSA: Set texture parameters without binding
    glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}

unsigned int TextureLoader::CreateGPUTextureCompressed(tinyddsloader::DDSFile& ddsimg) {
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

    // DSA: Create texture without binding
    glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

    // Calculate mipmap levels
    GLsizei width = static_cast<GLsizei>(ddsimg.GetWidth());
    GLsizei height = static_cast<GLsizei>(ddsimg.GetHeight());
    GLsizei levels = 1 + static_cast<GLsizei>(floor(log2(std::max(width, height))));

    // DSA: Allocate compressed immutable storage
    glTextureStorage2D(textureID, levels, glCompressedFormat, width, height);

    // DSA: Upload compressed pixel data without binding
    glCompressedTextureSubImage2D(textureID, 0, 0, 0, width, height, glCompressedFormat,
        static_cast<GLsizei>(ddsimg.GetImageData(0,0)->m_memSlicePitch),
        ddsimg.GetImageData(0,0)->m_mem);

    // DSA: Generate mipmaps without binding
    glGenerateTextureMipmap(textureID);

    // DSA: Set texture parameters without binding
    glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

    // Validate all faces first
    for (int i = 0; i < 6; ++i)
    {
        if (!data.faces[i].isValid || !data.faces[i].pixels)
        {
            spdlog::error("Invalid face data for cubemap face {}", i);
            return 0;
        }
    }

    // Determine format from first face (assume all faces have same format)
    const auto &face0 = data.faces[0];
    GLenum internalFormat, dataFormat;
    if (face0.channels == 1)
    {
        internalFormat = GL_R8;  // DSA requires sized format
        dataFormat = GL_RED;
    }
    else if (face0.channels == 3)
    {
        internalFormat = GL_RGB8;  // DSA requires sized format
        dataFormat = GL_RGB;
    }
    else if (face0.channels == 4)
    {
        internalFormat = GL_RGBA8;  // DSA requires sized format
        dataFormat = GL_RGBA;
    }
    else
    {
        internalFormat = GL_RGB8;  // DSA requires sized format (default fallback)
        dataFormat = GL_RGB;
    }

    unsigned int textureID;

    // DSA: Create cubemap texture without binding
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureID);

    // Calculate mipmap levels if needed
    GLsizei levels = 1;
    if (generateMipmaps)
    {
        levels = 1 + static_cast<GLsizei>(floor(log2(std::max(face0.width, face0.height))));
    }

    // DSA: Allocate immutable storage for all 6 faces
    glTextureStorage2D(textureID, levels, internalFormat, face0.width, face0.height);

    // Load all 6 faces using DSA
    for (int i = 0; i < 6; ++i)
    {
        const auto &face = data.faces[i];

        // DSA: Upload face data as a layer in the cubemap (faces are layers 0-5)
        glTextureSubImage3D(textureID, 0, 0, 0, i, face.width, face.height, 1,
            dataFormat, GL_UNSIGNED_BYTE, face.pixels);
    }

    // DSA: Set cubemap parameters without binding
    glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(textureID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    if (generateMipmaps)
    {
        // DSA: Generate mipmaps without binding
        glGenerateTextureMipmap(textureID);
        glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        // Enable anisotropic filtering for cubemaps with mipmaps
        GLfloat maxAnisotropy = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        glTextureParameterf(textureID, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
    }

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

unsigned int TextureLoader::CubemapFromTextureIDs(
    const std::array<unsigned int, 6>& textureIDs,
    bool generateMipmaps)
{
    struct SourceTextureInfo {
        unsigned int id = 0;
        GLint target = GL_NONE;
        GLint width = 0;
        GLint height = 0;
        GLint internalFormat = GL_NONE;
        GLint compressed = GL_FALSE;
    };

    auto consumePendingErrors = []() {
        while (glGetError() != GL_NO_ERROR) {
        }
    };

    auto logValidationFailure = [](int face,
                                   const SourceTextureInfo& info,
                                   const char* reason) {
        spdlog::error(
            "CubemapFromTextureIDs: Validation failed for face {} (texture {}, target 0x{:X}, size {}x{}, internal format 0x{:X}, compressed {}) - {}",
            face,
            info.id,
            static_cast<unsigned int>(info.target),
            info.width,
            info.height,
            static_cast<unsigned int>(info.internalFormat),
            info.compressed == GL_TRUE,
            reason
        );
    };

    std::array<SourceTextureInfo, 6> sourceTextures{};
    GLint previousTexture2DBinding = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture2DBinding);

    for (int i = 0; i < 6; ++i) {
        SourceTextureInfo& info = sourceTextures[i];
        info.id = textureIDs[i];

        if (info.id == 0) {
            logValidationFailure(i, info, "texture ID is 0");
            return 0;
        }

        consumePendingErrors();
        glBindTexture(GL_TEXTURE_2D, info.id);
        GLenum bindError = glGetError();
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture2DBinding));

        if (bindError != GL_NO_ERROR) {
            spdlog::error(
                "CubemapFromTextureIDs: Failed to validate source texture target for face {} (texture {}, expected GL_TEXTURE_2D, OpenGL error: 0x{:X})",
                i,
                info.id,
                bindError
            );
            return 0;
        }

        info.target = GL_TEXTURE_2D;

        consumePendingErrors();
        glGetTextureLevelParameteriv(info.id, 0, GL_TEXTURE_WIDTH, &info.width);
        glGetTextureLevelParameteriv(info.id, 0, GL_TEXTURE_HEIGHT, &info.height);
        glGetTextureLevelParameteriv(info.id, 0, GL_TEXTURE_INTERNAL_FORMAT, &info.internalFormat);
        glGetTextureLevelParameteriv(info.id, 0, GL_TEXTURE_COMPRESSED, &info.compressed);

        GLenum queryError = glGetError();
        if (queryError != GL_NO_ERROR) {
            spdlog::error(
                "CubemapFromTextureIDs: Failed to query source texture for face {} (texture {}, OpenGL error: 0x{:X})",
                i,
                info.id,
                queryError
            );
            return 0;
        }

        if (info.width <= 0 || info.height <= 0) {
            logValidationFailure(i, info, "source texture has invalid level 0 dimensions");
            return 0;
        }

        if (info.width != info.height) {
            logValidationFailure(i, info, "cubemap faces must be square");
            return 0;
        }

        if (info.internalFormat == GL_NONE) {
            logValidationFailure(i, info, "source texture reported an invalid internal format");
            return 0;
        }

        if (i == 0) {
            continue;
        }

        const SourceTextureInfo& reference = sourceTextures[0];
        if (info.width != reference.width || info.height != reference.height) {
            logValidationFailure(i, info, "all source textures must have the same dimensions as face 0");
            return 0;
        }

        if (info.internalFormat != reference.internalFormat) {
            logValidationFailure(i, info, "all source textures must have the same internal format as face 0");
            return 0;
        }

        if (info.compressed != reference.compressed) {
            logValidationFailure(i, info, "all source textures must have the same compression state as face 0");
            return 0;
        }
    }

    const SourceTextureInfo& face0 = sourceTextures[0];
    unsigned int cubemapID = 0;
    GLsizei levels = 1;
    if (generateMipmaps) {
        levels = 1 + static_cast<GLsizei>(floor(log2(std::max(face0.width, face0.height))));
    }

    consumePendingErrors();
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &cubemapID);
    glTextureStorage2D(cubemapID, levels, static_cast<GLenum>(face0.internalFormat), face0.width, face0.height);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR || cubemapID == 0) {
        spdlog::error(
            "CubemapFromTextureIDs: Failed to create cubemap storage (OpenGL error: 0x{:X}, size {}x{}, internal format 0x{:X}, compressed {})",
            error,
            face0.width,
            face0.height,
            static_cast<unsigned int>(face0.internalFormat),
            face0.compressed == GL_TRUE
        );
        if (cubemapID != 0) {
            glDeleteTextures(1, &cubemapID);
        }
        return 0;
    }

    // Order: +X (right), -X (left), +Y (top), -Y (bottom), +Z (back), -Z (front)
    for (int i = 0; i < 6; ++i) {
        // Swap Top (+Y, index 2) and Bottom (-Y, index 3) to compensate for Y-negation in shader.
        // This preserves the existing editor assignment behavior.
        int sourceIndex = i;
        if (i == 2) sourceIndex = 3;
        else if (i == 3) sourceIndex = 2;

        const SourceTextureInfo& source = sourceTextures[sourceIndex];

        consumePendingErrors();
        glCopyImageSubData(
            source.id,
            GL_TEXTURE_2D,
            0,
            0, 0, 0,
            cubemapID,
            GL_TEXTURE_CUBE_MAP,
            0,
            0, 0, i,
            source.width,
            source.height,
            1
        );

        error = glGetError();
        if (error != GL_NO_ERROR) {
            spdlog::error(
                "CubemapFromTextureIDs: Failed to copy source face {} into cubemap face {} (texture {}, target 0x{:X}, size {}x{}, internal format 0x{:X}, compressed {}) (OpenGL error: 0x{:X})",
                sourceIndex,
                i,
                source.id,
                static_cast<unsigned int>(source.target),
                source.width,
                source.height,
                static_cast<unsigned int>(source.internalFormat),
                source.compressed == GL_TRUE,
                error
            );
            glDeleteTextures(1, &cubemapID);
            return 0;
        }
    }

    glTextureParameteri(cubemapID, GL_TEXTURE_MIN_FILTER, generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTextureParameteri(cubemapID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(cubemapID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cubemapID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cubemapID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    if (generateMipmaps) {
        consumePendingErrors();
        glGenerateTextureMipmap(cubemapID);

        error = glGetError();
        if (error != GL_NO_ERROR) {
            spdlog::error(
                "CubemapFromTextureIDs: Failed to generate cubemap mipmaps (texture {}, size {}x{}, internal format 0x{:X}, compressed {}) (OpenGL error: 0x{:X})",
                cubemapID,
                face0.width,
                face0.height,
                static_cast<unsigned int>(face0.internalFormat),
                face0.compressed == GL_TRUE,
                error
            );
            glDeleteTextures(1, &cubemapID);
            return 0;
        }
    }

    spdlog::info(
        "CubemapFromTextureIDs: Successfully created cubemap (ID: {}) from 6 textures ({}x{}, internal format 0x{:X}, compressed {})",
        cubemapID,
        face0.width,
        face0.height,
        static_cast<unsigned int>(face0.internalFormat),
        face0.compressed == GL_TRUE
    );

    return cubemapID;
}
