#pragma once

#include <glad/gl.h>
#include <string>

// Simple texture struct following LearnOpenGL approach
struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

// Simple texture loading function
unsigned int TextureFromFile(const char *path, const std::string &directory, bool gamma = false);