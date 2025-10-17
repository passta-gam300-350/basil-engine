/******************************************************************************/
/*!
\file   MonoLoader.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the declaration for the MonoLoader class, which
is responsible for initializing and managing the Mono runtime environment.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MONOLOADER_HPP
#define MONOLOADER_HPP

#include <string>
#include <memory>
#include <mono/metadata/image.h>
#include <mono/utils/mono-forward.h>
struct ManagedAssembly;
class MonoLoader {


	std::string mono_dir;
	MonoDomain* backendDomain;
	MonoDomain* gameDomain;
	MonoDomain* compilerDomain;

public:

	MonoLoader() = default;

	void Initialize(std::string const& assembly_dir, std::string const& config_dir);
	std::unique_ptr<ManagedAssembly> LoadAssembly(std::string const& assemblyPath, MonoDomain* domain);
	MonoImage* LoadImage(ManagedAssembly const& assembly);

	MonoDomain* GetBackendDomain();
	MonoDomain* GetGameDomain();
	MonoDomain* GetCompilerDomain();

	MonoDomain* GetActiveDomain();


	


	void Enable_BackEnd();
	void Enable_Game();
	void Enable_Compiler();
	void Exit();

	~MonoLoader();

};

#endif // MONOLOADER_HPP