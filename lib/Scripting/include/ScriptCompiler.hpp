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

	std::unique_ptr<ManagedAssembly> compilerAssembly;
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

	struct CompileSettings {
		std::string output_name = "GameAssembly";
		std::string output_directory = "bin";
		std::string project_output_dir = "bin";
		bool isDLL = true;
		bool debug = true;
		bool optimize = false;
		bool verbose = false;
	} compile_settings;


	CommandInfo command_info{ "csc", false };


	std::vector<ScriptInfo> scripts;
	std::vector<std::thread> processes;

	
	std::string compiler_path;
	bool m_LastCompileSucceeded = true;
	std::string m_LastCompileFailure;

public:

	struct DiagnosticLog {
		std::string filename;
		std::string position;
		std::string severity;
		std::string diagnosticID;
		std::string message;

		friend std::ostream& operator<<(std::ostream& os, const ScriptCompiler::DiagnosticLog& log);

		
	};
	
	
	std::vector<DiagnosticLog> diagnostics;

	void Init(MonoLoader* loader, std::string const& compiler_dir);
	void CompileAsync();
	void CompileAllScripts();

	std::vector<std::string> const& GetLogs() const;
	bool LastCompileSucceeded() const;
	std::string const& GetLastCompileFailure() const;

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



	void SetCompileOutputName(std::string const& name);
	std::string const& GetCompileOutputName() const;
	void SetCompileOutputDirectory(std::string const& dir);
	void SetProjectOutputDirectory(std::string const& dir);
	std::string const& GetCompileOutputDirectory() const;
	void SetCompileAsDLL(bool isDLL);
	bool IsCompileAsDLL() const;
	void SetOptimizeCompile(bool optimize);
	bool IsOptimizeCompile() const;
	bool IsVerboseCompile() const;
	void SetVerboseCompile(bool verbose);


	~ScriptCompiler();







};



#endif // SCRIPTCOMPILER_HPP
