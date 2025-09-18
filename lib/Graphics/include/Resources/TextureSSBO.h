#pragma once

#include "Buffer/ShaderStorageBuffer.h"
#include <glad/glad.h>
#include <vector>
#include <cstdint>

// Data structure for SSBO (std430 aligned)
struct TextureHandleData {
    GLuint64 handle;    // 8 bytes - bindless texture handle
    uint32_t type;      // 4 bytes - texture type (0=diffuse, 1=normal, etc.)
    uint32_t flags;     // 4 bytes - texture flags
    // Total: 16 bytes per handle (properly aligned)
};

class TextureSSBO {
public:
    static constexpr uint32_t MAX_TEXTURES = 1024;
    static constexpr uint32_t BINDING_POINT = 1;

    TextureSSBO();
    ~TextureSSBO() = default;

    // Update SSBO with new texture data
    void UpdateData(const std::vector<TextureHandleData>& handleData);

    // Bind SSBO to shader
    void Bind();

private:
    TypedShaderStorageBuffer<uint32_t> m_SSBO;
    bool m_Initialized = false;

    void EnsureInitialized();
};