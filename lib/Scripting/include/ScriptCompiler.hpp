/******************************************************************************/
/*!
\file   ScriptCompiler.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the declaration for the ScriptCompiler class, which
is responsible for compiling C# scripts using the Mono runtime.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef SCRIPTCOMPILER_HPP
#define SCRIPTCOMPILER_HPP

#include <string>

#include <vector>
#include <thread>
#include <unordered_map>

#include "MonoLoader.hpp"


class ScriptCompiler {

	MonoLoader* loader;

	MonoAssembly* compilerAssembly;
	MonoImage* compilerImage;

	MonoClass* compilerKlass;

	MonoMethod* current_method;



	size_t max_thread; //!< Maximum number of concurrent threads for compilation
	size_t max_script_thread; //!< Maximum number of scripts to compile concurrently (-1 for no limit)
	size_t current_running_thread; //!< Current number of running threads

	std::vector<std::string> log;
	std::unordered_map<std::string,std::vector<std::string>> search_directories;
	std::unordered_map<std::string, std::string> assemblies_references; //!< Global assembly references

	struct ScriptInfo {
		std::string path;
		std::string name;
	};
	struct CommandInfo {
		std::string command;
		bool debug;

	};


	CommandInfo command_info{ "csc", false };


	std::vector<ScriptInfo> scripts;
	std::vector<std::thread> processes;

	
	std::string compiler_path;

public:
	void Init(MonoLoader* loader, std::string const& compiler_dir);
	void CompileAsync();
	void CompileAllScripts();

	std::vector<std::string> const& GetLogs() const;

	void SetMaxThread(size_t max);
	size_t GetMaxThread() const;
	size_t GetCurrentThreadID() const;
	size_t GetMaxScriptThread() const;
	void SetMaxScriptThread(size_t max);
	
	void SetDebugCompile(bool debug);
	bool IsDebugCompile() const;



	void GetManagedLogs();
	void ClearLog();

	void AddSearchDirectories(std::string const& id, std::string const& path);


	void CollectScripts(std::string const& id ="DEFAULT");

	void AddScript(std::string const& path);
	void RemoveScript(std::string const& path);
	void ClearScripts();


	void WaitAll();

	void EndProcesses();

	std::vector<ScriptInfo> const& GetScripts() const;



	void printLog() const;


	void AddReferences(std::string const& name, std::string const& path);
	void RemoveReferences(std::string const& name);
	void ClearReferences();






};
#endif // SCRIPTCOMPILER_HPP