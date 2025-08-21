#pragma once

#include "Shader.h"
#include <unordered_map>
#include <string>
#include <memory>

class Shaderlibrary
{
public:

	Shaderlibrary() = default;
	~Shaderlibrary() = default;
	
	// Add shaders to the library
	void Add(std::string const &name, std::shared_ptr<Shader> const& shader);
	void Add(std::shared_ptr<Shader> const &shader); // Uses shader's internal name

	// Load shaders from filepath
	std::shared_ptr<Shader> Load(std::string const &filepath);
	std::shared_ptr<Shader> Load(std::string const& name, std::string const &filepath);

	// Retrieve shaders
	std::shared_ptr<Shader> Get(std::string const &name);

	// Utility functions
	bool Exists(std::string const& name) const;
	void Remove(std::string const& name);
	void Clear();

	// Additional helpers (useful for debugging/UI)
	std::vector<std::string> GetShaderNames() const;

private:
	
	std::string ExtractNameFromFilePath(std::string const &filepath);
	std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
};