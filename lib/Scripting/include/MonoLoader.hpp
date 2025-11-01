#ifndef MONOLOADER_HPP
#define MONOLOADER_HPP

#include <string>
#include <mono/metadata/image.h>
#include <mono/utils/mono-forward.h>

class MonoLoader {


	std::string mono_dir;
	MonoDomain* backendDomain;
	MonoDomain* gameDomain;
	MonoDomain* compilerDomain;

public:

	MonoLoader() = default;

	void Initialize(std::string const& assembly_dir, std::string const& config_dir);
	MonoAssembly* LoadAssembly(std::string const& assemblyPath);
	MonoImage* LoadImage(MonoAssembly* assembly);

	MonoDomain* GetBackendDomain();
	MonoDomain* GetGameDomain();
	MonoDomain* GetCompilerDomain();

	MonoDomain* GetActiveDomain();


	


	void Enable_BackEnd();
	void Enable_Game();
	void Enable_Compiler();
	void Exit();

};

#endif // MONOLOADER_HPP