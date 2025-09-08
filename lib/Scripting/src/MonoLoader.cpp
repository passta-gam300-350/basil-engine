#include "MonoLoader.hpp"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>



void MonoLoader::Initialize(std::string const& assembly_dir, std::string const& config_dir)
{
	mono_set_dirs(assembly_dir.c_str(), config_dir.c_str());

	backendDomain = mono_jit_init("BackendDomain");

	char name[255] = "GameDomain";
	gameDomain = mono_domain_create_appdomain(name, nullptr);

	char name2[255] = "CompilerDomain";
	compilerDomain = mono_domain_create_appdomain(name2, nullptr);



	


}

void MonoLoader::Enable_BackEnd()
{
	mono_domain_set(backendDomain, false);
}

void MonoLoader::Enable_Game()
{
	mono_domain_set(gameDomain, false);
}
void MonoLoader::Enable_Compiler()
{
	mono_domain_set(compilerDomain, false);
}

void MonoLoader::Exit()
{
	if (backendDomain)
	{
		mono_domain_unload(backendDomain);
		backendDomain = nullptr;
	}

	if (gameDomain)
	{
		mono_domain_unload(gameDomain);
		gameDomain = nullptr;
	}
}

MonoAssembly* MonoLoader::LoadAssembly(std::string const& assemblyPath)
{
	MonoAssembly* assembly = mono_domain_assembly_open(mono_domain_get(), assemblyPath.c_str());
	return assembly;
}
MonoImage* MonoLoader::LoadImage(MonoAssembly* assembly)
{
	if (!assembly)
		return nullptr;
	MonoImage* image = mono_assembly_get_image(assembly);
	return image;
}
MonoDomain* MonoLoader::GetBackendDomain()
{
	return backendDomain;
}
MonoDomain* MonoLoader::GetGameDomain()
{
	return gameDomain;
}
MonoDomain* MonoLoader::GetCompilerDomain()
{
	return compilerDomain;
}


MonoDomain* MonoLoader::GetActiveDomain()
{
	return mono_domain_get();
}

