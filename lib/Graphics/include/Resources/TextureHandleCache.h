#pragma once

#include <glad/glad.h>
#include <unordered_map>

class TextureHandleCache {
public:
    TextureHandleCache() = default;
    ~TextureHandleCache();

    // Get or create a bindless handle for a texture
    GLuint64 GetHandle(GLuint textureId);

    // Make all cached handles non-resident (for cleanup)
    void ClearAll();

    // Check if extension is supported
    static bool IsBindlessSupported();

private:
    std::unordered_map<GLuint, GLuint64> m_HandleCache;

    GLuint64 CreateHandle(GLuint textureId);
    void MakeResident(GLuint64 handle);
    void MakeNonResident(GLuint64 handle);
};