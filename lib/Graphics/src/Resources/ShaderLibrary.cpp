#include <Resources/ShaderLibrary.h>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

void Shaderlibrary::Add(std::string const &name, std::shared_ptr<Shader> const &shader)
{
	if (Exists(name))
	{
		std::cerr << "Shader already exists: " << name << std::endl;
		return;
	}

	m_Shaders[name] = shader;
	std::cout << "Added shader to library: " << name << std::endl;
}

void Shaderlibrary::Add(std::shared_ptr<Shader> const &shader)
{
	// Since Shader doesn't have a GetName() method anymore,
	// this overload requires the user to use the Add(name, shader) version
	std::cerr << "Cannot add shader without specifying a name. Use Add(name, shader) instead." << std::endl;
}

std::shared_ptr<Shader> Shaderlibrary::Load(std::string const &filepath)
{
	auto name = ExtractNameFromFilePath(filepath);
	return Load(name, filepath);
}

std::shared_ptr<Shader> Shaderlibrary::Load(std::string const &name, std::string const &filepath)
{
	// Check if already loaded
	if (Exists(name))
	{
		std::cout << "Shader already loaded: " << name << std::endl;
		return Get(name);
	}

	// Check if file exists
	if (!fs::exists(filepath))
	{
		std::cerr << "Shader file not found: " << filepath << std::endl;
		return nullptr;
	}

	// Shader constructor needs vertex and fragment paths separately
	// For now, assume filepath is the base name and we add .vert and .frag extensions
	std::string vertPath = filepath + ".vert";
	std::string fragPath = filepath + ".frag";
	
	if (!fs::exists(vertPath) || !fs::exists(fragPath))
	{
		// Try without extensions if they don't exist
		vertPath = filepath;
		fragPath = filepath;
	}
	
	try
	{
		auto shader = std::make_shared<Shader>(vertPath.c_str(), fragPath.c_str());
		Add(name, shader);
		return shader;
	}
	catch (std::exception const &e)
	{
		std::cerr << "Failed to load shader " << name << ": " << e.what() << std::endl;
		return nullptr;
	}
}

std::shared_ptr<Shader> Shaderlibrary::Load(std::string const& vertexSrc, std::string const& fragmentSrc, std::string const& name)
{
	// Check if already loaded
	if (Exists(name))
	{
		std::cout << "Shader already loaded: " << name << std::endl;
		return Get(name);
	}

	// Shader constructor expects file paths, not source strings
	// This needs to be vertex and fragment file paths
	try
	{
		auto shader = std::make_shared<Shader>(vertexSrc.c_str(), fragmentSrc.c_str());
		Add(name, shader);
		return shader;
	}
	catch (std::exception const &e)
	{
		std::cerr << "Failed to load shader " << name << " from paths: " << e.what() << std::endl;
		return nullptr;
	}
}

std::shared_ptr<Shader> Shaderlibrary::Get(std::string const &name)
{
	if (!Exists(name))
	{
		std::cerr << "Shader not found: " << name << std::endl;
		return nullptr;
	}

	return m_Shaders[name];
}

bool Shaderlibrary::Exists(std::string const &name) const
{
	return m_Shaders.find(name) != m_Shaders.end();
}

void Shaderlibrary::Remove(std::string const &name)
{
	auto it = m_Shaders.find(name);
	if (it != m_Shaders.end())
	{
		m_Shaders.erase(it);
		std::cout << "Removed shader from library: " << name << std::endl;
	}
}

void Shaderlibrary::Clear()
{
	m_Shaders.clear();
	std::cout << "Cleared shader library" << std::endl;
}

std::vector<std::string> Shaderlibrary::GetShaderNames() const
{
	std::vector<std::string> names;
	names.reserve(m_Shaders.size());

	for (const auto &pair : m_Shaders)
	{
		names.push_back(pair.first);
	}

	return names;
}

std::string Shaderlibrary::ExtractNameFromFilePath(std::string const &filepath)
{
	return fs::path(filepath).stem().string();
}