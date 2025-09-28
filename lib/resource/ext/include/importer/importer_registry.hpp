#ifndef LIB_RESOURCE_IMPORTER_REGISTRY_H
#define LIB_RESOURCE_IMPORTER_REGISTRY_H

#include <string>

//addresses what is imported
namespace Resource {
	struct ImporterRegistry {
	private:
		std::string m_ImportedPath;
		ImporterRegistry() = default;
	public:
		static ImporterRegistry& Instance() {
			static ImporterRegistry instance{};
			return instance;
		}
		static std::string const& GetImportDirectory() {
			return Instance().m_ImportedPath;
		}
		static void SetImportDirectory(std::string const& path) {
			Instance().m_ImportedPath = path;
		}
	};
}

#endif