/******************************************************************************/
/*!
\file   MonoLoader.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contain the implmentation for the MonoLoader class, which
is responsible for initializing and managing the Mono runtime environment.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "MonoLoader.hpp"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include "ABI/ABI.h"

#include <cassert>
#include <mono/metadata/threads.h>


void MonoLoader::Initialize(std::string const& assembly_dir, std::string const& config_dir)
{
	mono_set_dirs(assembly_dir.c_str(), config_dir.c_str());

	backendDomain = mono_jit_init("BackendDomain");

	char name[255] = "GameDomain";
	gameDomain = mono_domain_create_appdomain(name, nullptr);

	char name2[255] = "CompilerDomain";
	compilerDomain = mono_domain_create_appdomain(name2, nullptr);

	mono_thread_attach(backendDomain);

	


}

void MonoLoader::Enable_BackEnd()
{
	MonoThread* currentThread = mono_thread_current();
	if (!currentThread)
	{
		currentThread = mono_thread_attach(mono_get_root_domain());
	}
	mono_domain_set(backendDomain, false);
}

void MonoLoader::Enable_Game()
{
	MonoThread* currentThread = mono_thread_current();
	if (!currentThread)
	{
		currentThread = mono_thread_attach(mono_get_root_domain());
	}
	mono_domain_set(gameDomain, false);
}
void MonoLoader::Enable_Compiler()
{
	mono_domain_set(compilerDomain, false);
}

void MonoLoader::Exit()
{

	if (gameDomain)
	{
		mono_domain_unload(gameDomain);
		gameDomain = nullptr;
	}
	if (compilerDomain)
	{
		mono_domain_unload(compilerDomain);
		compilerDomain = nullptr;
	}

	if (backendDomain)
	{
		mono_jit_cleanup(backendDomain);
		backendDomain = nullptr;
	}
}

std::unique_ptr<ManagedAssembly> MonoLoader::LoadAssembly(std::string const& assemblyPath, MonoDomain* domain)
{
	auto assembly = std::make_unique<ManagedAssembly>(std::make_unique<CSAssembly>(assemblyPath));
	assembly->Load(domain);
	return assembly;
}
MonoImage* MonoLoader::LoadImage(const ManagedAssembly& assembly)
{
	assert(assembly.IsLoaded() && "Assembly is not loaded!");
	return assembly.IsLoaded() ? assembly.Image() : nullptr;
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

MonoLoader::~MonoLoader()
{
	Exit();

}