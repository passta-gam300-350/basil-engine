#pragma once

#include <string>
#include <glad/gl.h>
#include <unordered_map>
#include <glm/glm.hpp>

class Shader
{
public:
	Shader(std::string const &vertexSrc, std::string const &fragmentSrc);
	Shader(std::string const &filepath);
	~Shader();

	void Bind() const;
	void Unbind() const;

	void SetInt(std::string const &name, int value);
	void SetIntArray(std::string const &name, int *values, uint32_t count);
	void SetFloat(std::string const &name, float value);
	void SetFloat2(std::string const &name, glm::vec2 const &value);
	void SetFloat3(std::string const &name, glm::vec3 const &value);
	void SetFloat4(std::string const &name, glm::vec4 const &value);
	void SetMat3(std::string const &name, glm::mat3 const &matrix);
	void SetMat4(std::string const &name, glm::mat4 const&matrix);

	uint32_t GetShdrPgmHandle () const
	{
		return m_ShdrPgmHandle;
	}
	const std::string &GetName() const
	{
		return m_Name;
	}



private:
	std::string ReadFile(const std::string &filepath);
	std::unordered_map<uint32_t, std::string> PreProcess(const std::string &source);
	void Compile(const std::unordered_map<uint32_t, std::string> &shaderSources);
	GLint GetUniformLocation(std::string const &name);

	uint32_t m_ShdrPgmHandle;
	std::string m_Name;
	std::unordered_map<std::string, int> m_UniformLocationCache;
};