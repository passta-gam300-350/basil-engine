#include "MonoManager.hpp"

#include <filesystem>
#include <iostream>
#include <mono/jit/jit.h>

#include "MonoLoader.hpp"
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-debug.h>

#include "ScriptCompiler.hpp"

namespace
{

	static char const* assembly_dir = MONO_ASSEMBLY_PATH;
	static char const* config_dir = MONO_CONFIG_PATH;
	static char const* csc_path = MONO_COMPILER;
}


ScriptCompiler* MonoManager::m_Compiler = nullptr;
MonoLoader* MonoManager::m_Loader = nullptr;
std::vector<std::string> MonoManager::m_ScriptBins  {};
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

	m_Compiler->SetDebugCompile(true);
	m_Compiler->SetMaxThread(4);
	m_Compiler->SetMaxScriptThread(-1);

	
}

void MonoManager::AddSearchDirectories(std::string const& path)
{
	std::filesystem::path file_path = path;
	std::cout <<std::filesystem::absolute(file_path).string() << std::endl;
	if (std::filesystem::is_directory(file_path)) {
		if (m_Verbose) {
			std::cout << "Adding script bin directory: " << std::filesystem::absolute(file_path).string() << std::endl;

		}
		m_Compiler->AddSearchDirectories("DEFAULT",std::filesystem::absolute(file_path).string());
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



