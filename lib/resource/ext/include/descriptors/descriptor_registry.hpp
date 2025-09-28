#ifndef LIB_RESOURCE_DESCRIPTOR_REGISTRY_H
#define LIB_RESOURCE_DESCRIPTOR_REGISTRY_H

#include <string>
#include <filesystem>

namespace Resource {
	struct DescriptorRegistry {
	private:
		std::string m_DescriptorRootPath; //fix this. here for portability temp fix
		DescriptorRegistry() = default;
	public:
		static DescriptorRegistry& Instance() {
			static DescriptorRegistry instance{};
			return instance;
		}
		static std::string const& GetDescriptorRootDirectory() {
			return Instance().m_DescriptorRootPath;
		}
		static void SetDescriptorRootDirectory(std::string const& path) {
			Instance().m_DescriptorRootPath = std::filesystem::path(path).lexically_normal().make_preferred().string();
		}
	};
}

#endif