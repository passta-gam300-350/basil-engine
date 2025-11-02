/******************************************************************************/
/*!
\file   MonoManager.cpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contain the implmentation for the MonoManager class, which
is a singleton that manages the Mono runtime, including the ScriptCompiler and MonoLoader instances.
It is responsible for initializing the Mono environment, handling script compilation,
and managing script binaries. Furthermore, it manage the C# instances and their lifecycles.


Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "MonoManager.hpp"

#include <filesystem>
#include <iostream>
#include <mono/jit/jit.h>

#include "MonoLoader.hpp"
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-debug.h>

#include "ScriptCompiler.hpp"
#include "ABI/ABI.h"
#include <mono/metadata/threads.h>

namespace
{

	static char const* assembly_dir = MONO_ASSEMBLY_PATH;
	static char const* config_dir = MONO_CONFIG_PATH;
	static char const* csc_path = MONO_COMPILER;
}


ScriptCompiler* MonoManager::m_Compiler = nullptr;
MonoLoader* MonoManager::m_Loader = nullptr;
std::vector<std::string> MonoManager::m_ScriptBins{};
bool MonoManager::m_Verbose = false;


void MonoManager::Initialize()
{

	//const char* options[] = {// Enable debugging
	//"--debugger-agent=transport=dt_socket,address=127.0.0.1:55555,server=y,suspend=n", // Debugger agent
	//};
	//mono_jit_parse_options(sizeof(options) / sizeof(char*), (char**)options);

	//mono_debug_init(MONO_DEBUG_FORMAT_MONO);
	m_Loader = new MonoLoader();
	m_Loader->Initialize(assembly_dir, config_dir);


	m_Compiler = new ScriptCompiler();
	m_Compiler->Init(m_Loader, csc_path);
	m_Compiler->AddReferences("engine", R"(C:\Users\yeo_j\Documents\Digipen Repo\Year 3\Project\Project\engine\managed\BasilEngine\bin\Release\net48\BasilEngine.dll)");

	m_Compiler->SetDebugCompile(true);
	m_Compiler->SetMaxThread(4);
	m_Compiler->SetMaxScriptThread(uint64_t(-1));
	m_Compiler->SetVerboseCompile(true);


}

void MonoManager::AddSearchDirectories(std::string const& path)
{
	std::filesystem::path file_path = path;
	std::cout << std::filesystem::absolute(file_path).string() << std::endl;
	if (std::filesystem::is_directory(file_path)) {
		if (m_Verbose) {
			std::cout << "Adding script bin directory: " << std::filesystem::absolute(file_path).string() << std::endl;

		}
		m_Compiler->AddSearchDirectories("DEFAULT", std::filesystem::absolute(file_path).string());
	}

}



void MonoManager::ClearBins()
{
	m_ScriptBins.clear();
}

std::vector<std::string> const& MonoManager::GetBins()
{
	return m_ScriptBins;
}

void MonoManager::SetVerbose(bool v)
{
	m_Verbose = v;
}

bool MonoManager::GetVerbose()
{
	return m_Verbose;


}

void MonoManager::StartCompilation()
{
	m_Compiler->CollectScripts();
	m_Compiler->CompileAllScripts();
	m_Compiler->WaitAll();
	m_Compiler->GetManagedLogs();
	m_Compiler->printLog();
}

ScriptCompiler* MonoManager::GetCompiler()
{
	return m_Compiler;
}
MonoLoader* MonoManager::GetLoader()
{
	return m_Loader;
}


std::shared_ptr<CSKlass> MonoManager::GetKlass(ManagedAssembly* assembly, const char* klassName, const char* klassNamespace)
{
	return std::make_shared<CSKlass>(assembly->Image(), klassNamespace, klassName);
}

std::unique_ptr<CSKlassInstance> MonoManager::CreateInstance(MonoDomain* domain, CSKlass const& klass, void* args[])
{
	return std::make_unique<CSKlassInstance>(klass.CreateInstance(domain, args));
}

std::vector<std::shared_ptr<CSKlass>> MonoManager::LoadKlassesFromAssembly(ManagedAssembly* assembly)
{
	std::vector<std::shared_ptr<CSKlass>> klasses;

	MonoImage* image = assembly->Image();
	if (!image)
	{
		return klasses;
	}
	const MonoTableInfo* typeDefTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
	if (!typeDefTable)
	{
		return klasses;
	}
	int rows = mono_table_info_get_rows(typeDefTable);

	for (int i = 0; i < rows; ++i)
	{
		uint32_t cols[MONO_TYPEDEF_SIZE];
		mono_metadata_decode_row(typeDefTable, i, cols, MONO_TYPEDEF_SIZE);
		uint32_t nameIdx = cols[MONO_TYPEDEF_NAME];
		uint32_t namespaceIdx = cols[MONO_TYPEDEF_NAMESPACE];
		const char* className = mono_metadata_string_heap(image, nameIdx);

		if (std::string{ className } == "<Module>")
			continue;

		const char* namespaceName = mono_metadata_string_heap(image, namespaceIdx);
		auto klass = std::make_shared<CSKlass>(image, namespaceName, className);
		
		klasses.push_back(klass);

	}


	return klasses;
}


void MonoManager::Attach() {

	MonoDomain* rootDomain = mono_get_root_domain();
	if (mono_domain_get() == rootDomain)
		return;
	mono_thread_attach(rootDomain);
	mono_domain_set(rootDomain, false);
}

void MonoManager::Detach() {
	MonoThread* thread = mono_thread_current();
	mono_thread_detach(thread);
}
MonoManager::~MonoManager() = default;



