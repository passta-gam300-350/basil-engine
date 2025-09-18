#include "Resources/TextureHandleCache.h"
#include <iostream>
#include <cstring>

TextureHandleCache::~TextureHandleCache() {
    ClearAll();
}

GLuint64 TextureHandleCache::GetHandle(GLuint textureId) {
    // Check cache first
    auto it = m_HandleCache.find(textureId);
    if (it != m_HandleCache.end()) {
        return it->second;
    }

    // Create new handle
    GLuint64 handle = CreateHandle(textureId);
    if (handle != 0) {
        m_HandleCache[textureId] = handle;
    }

    return handle;
}

void TextureHandleCache::ClearAll() {
    for (const auto& pair : m_HandleCache) {
        MakeNonResident(pair.second);
    }
    m_HandleCache.clear();
}

bool TextureHandleCache::IsBindlessSupported() {
    if (!glGetIntegerv || !glGetStringi) {
        return false;
    }

    GLint numExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

    if (glGetError() != GL_NO_ERROR || numExtensions <= 0) {
        return false;
    }

    for (GLint i = 0; i < numExtensions; ++i) {
        const GLubyte* extension = glGetStringi(GL_EXTENSIONS, i);
        if (!extension) continue;

        const char* extName = (const char*)extension;
        if (strcmp(extName, "GL_ARB_bindless_texture") == 0 ||
            strcmp(extName, "GL_NV_bindless_texture") == 0) {
            return true;
        }
    }

    return false;
}

GLuint64 TextureHandleCache::CreateHandle(GLuint textureId) {
    GLuint64 handle = glGetTextureHandleARB(textureId);
    if (handle != 0) {
        MakeResident(handle);
    }
    return handle;
}

void TextureHandleCache::MakeResident(GLuint64 handle) {
    if (handle != 0) {
        glMakeTextureHandleResidentARB(handle);
    }
}

void TextureHandleCache::MakeNonResident(GLuint64 handle) {
    if (handle != 0) {
        glMakeTextureHandleNonResidentARB(handle);
    }
}