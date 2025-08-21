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
	std::string const &name = shader->GetName();
	if (name.empty())
	{
		std::cerr << "Cannot add shader with empty name to library" << std::endl;
		return;
	}

	Add(name, shader);
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

	// Create and load shader
	try
	{
		auto shader = std::make_shared<Shader>(filepath);
		Add(name, shader);
		return shader;
	}
	catch (std::exception const &e)
	{
		std::cerr << "Failed to load shader " << name << ": " << e.what() << std::endl;
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