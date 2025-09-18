#include "Resources/TextureSSBO.h"
#include <iostream>

TextureSSBO::TextureSSBO()
    : m_SSBO(std::vector<uint32_t>(MAX_TEXTURES * 4, 0), GL_DYNAMIC_DRAW) { // 4 uint32_t per texture: uvec2 handle + type + flags
    m_Initialized = true; // Already initialized with proper size
}

void TextureSSBO::UpdateData(const std::vector<TextureHandleData>& handleData) {
    // The shader expects separated arrays, not interleaved data!
    // Layout: uvec2 handles[1024], uint types[1024], uint flags[1024]

    // Create separate arrays to match shader layout
    std::vector<uint32_t> handlesArray(MAX_TEXTURES * 2, 0); // uvec2 = 2 × uint32_t
    std::vector<uint32_t> typesArray(MAX_TEXTURES, 0);
    std::vector<uint32_t> flagsArray(MAX_TEXTURES, 0);

    // Fill arrays with actual data
    for (size_t i = 0; i < handleData.size() && i < MAX_TEXTURES; ++i) {
        // Convert 64-bit handle to uvec2 (2 × 32-bit)
        uint32_t low = static_cast<uint32_t>(handleData[i].handle & 0xFFFFFFFF);
        uint32_t high = static_cast<uint32_t>((handleData[i].handle >> 32) & 0xFFFFFFFF);
        handlesArray[i * 2] = low;      // uvec2.x
        handlesArray[i * 2 + 1] = high; // uvec2.y

        typesArray[i] = handleData[i].type;
        flagsArray[i] = handleData[i].flags;

    }

    // Create combined buffer with separated layout
    std::vector<uint32_t> combinedBuffer;
    combinedBuffer.reserve(MAX_TEXTURES * 4); // 2 for handles + 1 for types + 1 for flags

    // Add handles array (uvec2 format) - 2048 uint32_t (8192 bytes)
    combinedBuffer.insert(combinedBuffer.end(), handlesArray.begin(), handlesArray.end());

    // Add types array - 1024 uint32_t (4096 bytes)
    combinedBuffer.insert(combinedBuffer.end(), typesArray.begin(), typesArray.end());

    // Add flags array - 1024 uint32_t (4096 bytes)
    combinedBuffer.insert(combinedBuffer.end(), flagsArray.begin(), flagsArray.end());

    // Upload as raw uint32_t data
    m_SSBO.SetData(combinedBuffer);
}

void TextureSSBO::Bind() {
    m_SSBO.BindBase(BINDING_POINT);
}