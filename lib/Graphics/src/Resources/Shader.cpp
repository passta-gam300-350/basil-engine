#include"../../include/Resources/Shader.h"
#include <glad/gl.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

Shader::Shader(std::string const &vertexSrc, std::string const &fragmentSrc)
{
    // Create an unordered map of shader sources
    std::unordered_map<uint32_t, std::string> sources;
    sources[GL_VERTEX_SHADER] = vertexSrc;
    sources[GL_FRAGMENT_SHADER] = fragmentSrc;

    Compile(sources);
}

Shader::Shader(std::string const &filepath)
{
    std::string source = ReadFile(filepath);
    auto shaderSources = PreProcess(source);
    Compile(shaderSources);
}

Shader::~Shader()
{
    glDeleteProgram(m_ShdrPgmHandle);
}

std::string Shader::ReadFile(std::string const &filepath)
{
    std::string result;
    std::ifstream in(filepath, std::ios::in | std::ios::binary);

    if (in)
    {
        in.seekg(0, std::ios::end);
        result.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(result.data(), result.size());
        in.close();
    }
    else
    {
        std::cerr << "Could not open file: " << filepath << std::endl;
    }

    return result;
}

std::unordered_map<uint32_t, std::string> Shader::PreProcess(std::string const &source)
{
    std::unordered_map<uint32_t, std::string> shaderSources;

    const char *typeToken = "#type";
    size_t typeTokenLength = strlen(typeToken);
    size_t pos = source.find(typeToken, 0);

    while (pos != std::string::npos)
    {
        size_t eol = source.find_first_of("\r\n", pos);
        if (eol == std::string::npos)
        {
            std::cerr << "Syntax error in shader file!" << std::endl;
            return shaderSources;
        }

        size_t begin = pos + typeTokenLength + 1;
        std::string type = source.substr(begin, eol - begin);

        size_t nextLinePos = source.find_first_not_of("\r\n", eol);
        if (nextLinePos == std::string::npos)
        {
            std::cerr << "Syntax error in shader file!" << std::endl;
            return shaderSources;
        }

        pos = source.find(typeToken, nextLinePos);

        uint32_t shaderType = 0;
        if (type == "vertex")
        {
            shaderType = GL_VERTEX_SHADER;
        }
        else if (type == "fragment" || type == "pixel")
        {
            shaderType = GL_FRAGMENT_SHADER;
        }
        else
        {
            std::cerr << "Invalid shader type specified: " << type << std::endl;
            return shaderSources;
        }

        shaderSources[shaderType] = (pos == std::string::npos) ?
            source.substr(nextLinePos) :
            source.substr(nextLinePos, pos - nextLinePos);
    }

    return shaderSources;
}

void Shader::Compile(std::unordered_map<uint32_t, std::string> const &shaderSources)
{
    // Get a program ID from OpenGL
    GLuint program = glCreateProgram();

    if (shaderSources.size() > 2)
    {
        std::cerr << "Only supporting 2 shaders for now" << std::endl;
    }

    std::vector<GLenum> glShaderIDs;
    glShaderIDs.reserve(shaderSources.size());

    // Compile each shader
    for (auto &shdr : shaderSources)
    {
        GLenum type = shdr.first;
        std::string const &source = shdr.second;

        // Create a shader object
        GLuint shader = glCreateShader(type);

        // Set the shader source
        glShaderSource(shader, 1, &source.c_str(), nullptr);

        // Compile the shader
        glCompileShader(shader);

        // Check for shader compile errors
        GLint isCompiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

            // The maxLength includes the NULL character
            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog.data());

            // We don't need the shader anymore if it failed to compile
            glDeleteShader(shader);

            std::cerr << "Shader compilation failure!" << std::endl;
            std::cerr << infoLog.data() << std::endl;
            break;
        }

        // Attach the shader to our program
        glAttachShader(program, shader);
        glShaderIDs.push_back(shader);
    }

    // Link the program
    glLinkProgram(program);

    // Check for linking errors
    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(program, maxLength, &maxLength, infoLog.data());

        // We don't need the program anymore if it failed to link
        glDeleteProgram(program);

        // Delete the shaders as well
        for (auto id : glShaderIDs)
        {
            glDeleteShader(id);
        }

        std::cerr << "Shader linking failure!" << std::endl;
        std::cerr << infoLog.data() << std::endl;
        return;
    }

    // Always detach and delete shaders after a successful link
    for (auto id : glShaderIDs)
    {
        glDetachShader(program, id);
        glDeleteShader(id);
    }

    m_ShdrPgmHandle = program;
}

void Shader::Bind() const
{
    glUseProgram(m_ShdrPgmHandle);
}

void Shader::Unbind() const
{
    glUseProgram(0);
}

void Shader::SetInt(std::string const &name, int value)
{
    GLint location = GetUniformLocation(name);
    glUniform1i(location, value);
}

void Shader::SetIntArray(std::string const &name, int *values, uint32_t count)
{
    GLint location = GetUniformLocation(name);
    glUniform1iv(location, count, values);
}

void Shader::SetFloat(std::string const &name, float value)
{
    GLint location = GetUniformLocation(name);
    glUniform1f(location, value);
}

void Shader::SetFloat2(std::string const &name, glm::vec2 const &value)
{
    GLint location = GetUniformLocation(name);
    glUniform2f(location, value.x, value.y);
}

void Shader::SetFloat3(std::string const &name, glm::vec3 const &value)
{
    GLint location = GetUniformLocation(name);
    glUniform3f(location, value.x, value.y, value.z);
}

void Shader::SetFloat4(std::string const &name, glm::vec4 const &value)
{
    GLint location = GetUniformLocation(name);
    glUniform4f(location, value.x, value.y, value.z, value.w);
}

void Shader::SetMat3(std::string const &name, glm::mat3 const &matrix)
{
    GLint location = GetUniformLocation(name);
    glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::SetMat4(std::string const &name, glm::mat4 const &matrix)
{
    GLint location = GetUniformLocation(name);
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
}

GLint Shader::GetUniformLocation(std::string const &name)
{
    if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
        return m_UniformLocationCache[name];

    GLint location = glGetUniformLocation(m_ShdrPgmHandle, name.c_str());
    if (location == -1)
        std::cerr << "Warning: Uniform '" << name << "' doesn't exist!" << std::endl;

    m_UniformLocationCache[name] = location;
    return location;
}
