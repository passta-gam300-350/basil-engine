/******************************************************************************/
/*!
\file   MonoManager.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the declaration for the MonoManager class, which
is a singleton that manages the Mono runtime, including the ScriptCompiler and MonoLoader instances.
It is responsible for initializing the Mono environment, handling script compilation,
and managing script binaries. Furthermore, it manage the C# instances and their lifecycles.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MONOMANAGER_HPP
#define MONOMANAGER_HPP
#include <string>
#include <vector>
#include <memory>
#include <mono/utils/mono-forward.h>



class ScriptCompiler;
class MonoLoader;
struct CSKlass;
struct CSKlassInstance;
struct ManagedAssembly;

class MonoManager {
	static ScriptCompiler* m_Compiler;
	static MonoLoader* m_Loader;
	static std::vector<std::string> m_ScriptBins;

	static bool m_Verbose;



public:
	static void Initialize();
	static void AddSearchDirectories(std::string const& path);
	static void ClearBins();

	static std::vector<std::string> const& GetBins();

	static void SetVerbose(bool v);
	static bool GetVerbose();


	static void StartCompilation();


	static ScriptCompiler* GetCompiler();
	static MonoLoader* GetLoader();


	// Classes

	static std::shared_ptr<CSKlass> GetKlass(ManagedAssembly* assembly, const char* klassName, const char* klassNamespace = "");
	static std::unique_ptr<CSKlassInstance> CreateInstance(MonoDomain* domain, CSKlass const& klass, void* args[]=nullptr);

	// Reflection
	static std::vector<std::shared_ptr<CSKlass>> LoadKlassesFromAssembly(ManagedAssembly* assembly);
	~MonoManager();
};
#endif //MONOMANAGER_HPP