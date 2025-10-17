#include "ABI/CSAssembly.hpp"

#include <fstream>
#include <system_error>
#include <utility>

CSAssembly::CSAssembly(std::filesystem::path assemblyPath)
	: m_path(std::move(assemblyPath))
{
}

bool CSAssembly::Load()
{
	if (m_path.empty())
	{
		ClearLoadedData();
		return false;
	}

	return LoadFromFile();
}

bool CSAssembly::Load(const std::filesystem::path& path)
{
	m_path = path;
	return Load();
}

void CSAssembly::Unload() noexcept
{
	ClearLoadedData();
}

bool CSAssembly::IsLoaded() const noexcept
{
	return m_loaded;
}

const std::filesystem::path& CSAssembly::Path() const noexcept
{
	return m_path;
}

void CSAssembly::SetPath(std::filesystem::path path) noexcept
{
	if (m_path == path)
	{
		return;
	}

	m_path = std::move(path);
	ClearLoadedData();
}

const std::string& CSAssembly::Name() const noexcept
{
	return m_name;
}

std::size_t CSAssembly::Size() const noexcept
{
	return m_image.size();
}

const std::vector<std::uint8_t>& CSAssembly::Image() const noexcept
{
	return m_image;
}

bool CSAssembly::LoadFromFile()
{
	std::error_code ec;
	if (!std::filesystem::is_regular_file(m_path, ec))
	{
		ClearLoadedData();
		return false;
	}

	const auto fileSize = std::filesystem::file_size(m_path, ec);
	if (ec || fileSize == 0)
	{
		ClearLoadedData();
		return false;
	}

	m_image.resize(static_cast<std::size_t>(fileSize));

	std::ifstream file(m_path, std::ios::binary);
	if (!file.read(reinterpret_cast<char*>(m_image.data()), static_cast<std::streamsize>(fileSize)))
	{
		ClearLoadedData();
		return false;
	}

	m_name = m_path.stem().string();
	m_loaded = true;
	return true;
}

void CSAssembly::ClearLoadedData() noexcept
{
	m_image.clear();
	m_name.clear();
	m_loaded = false;
}

