#ifndef CSAssembly_HPP
#define CSAssembly_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct CSAssembly
{
	CSAssembly() = default;
	explicit CSAssembly(std::filesystem::path assemblyPath);

	bool Load();
	bool Load(const std::filesystem::path& path);
	void Unload() noexcept;

	[[nodiscard]] bool IsLoaded() const noexcept;
	[[nodiscard]] const std::filesystem::path& Path() const noexcept;
	void SetPath(std::filesystem::path path) noexcept;
	[[nodiscard]] const std::string& Name() const noexcept;
	[[nodiscard]] std::size_t Size() const noexcept;
	[[nodiscard]] const std::vector<std::uint8_t>& Image() const noexcept;

private:
	bool LoadFromFile();
	void ClearLoadedData() noexcept;

	std::filesystem::path m_path;
	std::string m_name;
	std::vector<std::uint8_t> m_image;
	bool m_loaded{ false };
};


#endif
