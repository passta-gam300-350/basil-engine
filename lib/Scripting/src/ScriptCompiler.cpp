/******************************************************************************/
/*!
\file   ScriptCompiler.cpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the implementation for the ScriptCompiler class, which
is responsible for compiling C# scripts using the Mono runtime.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "ABI/ABI.h"
#include "ScriptCompiler.hpp"
#include <filesystem>
#include <iostream>
#include <thread>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/class.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/object.h>
#include <mono/metadata/threads.h>

namespace {
bool EndsWith(const std::string& value, const std::string& suffix)
{
	return value.size() >= suffix.size() &&
		value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string MonoObjectToString(MonoObject* object)
{
	if (!object) {
		return {};
	}

	MonoObject* stringifyException = nullptr;
	MonoString* monoText = mono_object_to_string(object, &stringifyException);
	if (!monoText || stringifyException) {
		return "Managed exception occurred during script compilation.";
	}

	char* utf8 = mono_string_to_utf8(monoText);
	if (!utf8) {
		return "Managed exception occurred during script compilation.";
	}

	std::string result = utf8;
	mono_free(utf8);
	return result;
}

bool InvokeCompilerMethod(MonoMethod* method, void** args, std::string& failure)
{
	MonoObject* exception = nullptr;
	MonoObject* result = mono_runtime_invoke(method, nullptr, args, &exception);
	if (exception) {
		failure = MonoObjectToString(exception);
		return false;
	}

	if (!result) {
		return true;
	}

	MonoClass* resultClass = mono_object_get_class(result);
	if (resultClass == mono_get_boolean_class()) {
		void* raw = mono_object_unbox(result);
		if (!raw || *static_cast<mono_bool*>(raw) == 0) {
			failure = "Managed script compilation emit failed.";
			return false;
		}
	}

	return true;
}
}

void ScriptCompiler::Init(MonoLoader* ptrLoader, std::string const& compiler_dir)
{
	loader = ptrLoader;
	compiler_path = compiler_dir;

	loader->Enable_Compiler();
	compilerAssembly = loader->LoadAssembly(compiler_path, loader->GetCompilerDomain());
	compilerImage = loader->LoadImage(*compilerAssembly);

	compilerKlass = mono_class_from_name(compilerImage, "", "CSCompiler");
	/*
	 *public static void Setup(string output, string outputDirectory, string[] sources, string[] references, bool isDLL = false, bool debug = false,
	 *bool optimize = false, bool verbose = false)
	 */
	current_method = mono_class_get_method_from_name(compilerKlass, "Setup", 9);
	current_running_thread = 0;

}



std::vector<std::string> const& ScriptCompiler::GetLogs() const
{
	return log;
}

bool ScriptCompiler::LastCompileSucceeded() const
{
	return m_LastCompileSucceeded;
}

std::string const& ScriptCompiler::GetLastCompileFailure() const
{
	return m_LastCompileFailure;
}

size_t ScriptCompiler::GetCurrentThreadID() const
{
	return current_running_thread;
}

size_t ScriptCompiler::GetMaxScriptThread() const
{
	return max_script_thread;
}

void ScriptCompiler::SetMaxScriptThread(size_t max)
{
	max_script_thread = max;
}

void ScriptCompiler::SetMaxThread(size_t max)
{
	max_thread = max;
}

size_t ScriptCompiler::GetMaxThread() const
{
	return max_thread;
}

void ScriptCompiler::SetDebugCompile(bool debug)
{
	command_info.debug = debug;
}
bool ScriptCompiler::IsDebugCompile() const
{
	return command_info.debug;
}
void ScriptCompiler::ClearLog()
{
	log.clear();
	diagnostics.clear();
	m_LastCompileSucceeded = true;
	m_LastCompileFailure.clear();
}

void ScriptCompiler::AddScript(std::string const& path)
{
	std::filesystem::path file_path = path;

	/*if (!std::filesystem::exists(path))
	{
		return;
	}*/

	ScriptInfo info{ std::filesystem::absolute(file_path).string(), file_path.filename().string() };

	scripts.push_back(info);
}

void ScriptCompiler::ClearScripts()
{
	scripts.clear();
}

std::vector<ScriptCompiler::ScriptInfo> const& ScriptCompiler::GetScripts() const
{
	return scripts;
}

void ScriptCompiler::RemoveScript(std::string const& path)
{
	auto it = std::remove_if(scripts.begin(), scripts.end(), [&](ScriptInfo const& info) {
		return info.path == path;
	});
	scripts.erase(it, scripts.end());
}

void ScriptCompiler::CompileAsync()
{
	m_LastCompileSucceeded = true;
	m_LastCompileFailure.clear();

	MonoDomain* compiler_d = loader->GetCompilerDomain();
	// Build a arr of paths to be converted to mono string array
	std::vector<std::string> paths;

	for (const auto& script : scripts)
	{
		paths.push_back(script.path);
	}
	// Convert to mono string array
	MonoArray* mono_paths = mono_array_new(compiler_d, mono_get_string_class(), paths.size());
	for (size_t i = 0; i < paths.size(); ++i)
	{
		MonoString* mono_str = mono_string_new(compiler_d, paths[i].c_str());
		mono_array_setref(mono_paths, i, mono_str);
	}

	// No references for now
	MonoArray* mono_refs = mono_array_new(compiler_d, mono_get_string_class(), assemblies_references.size());
	for (size_t i = 0; i < assemblies_references.size(); ++i)
	{
		auto it = std::next(assemblies_references.begin(), i);
		MonoString* mono_str = mono_string_new(compiler_d, it->second.c_str());
		mono_array_setref(mono_refs, i, mono_str);

	}
	void* args[9];
	/*
	 *public static void Setup(string output, string outputDirectory, string[] sources, string[] references, bool isDLL = false, bool debug = false,
	 *bool optimize = false, bool verbose = false)
	*/
	MonoString* output = mono_string_new(compiler_d, compile_settings.output_name.c_str());
	MonoString* output_dir = mono_string_new(compiler_d, compile_settings.output_directory.c_str());
	MonoString* output_proj = mono_string_new(compiler_d, compile_settings.project_output_dir.c_str());
	//[[maybe_unused]] mono_bool val = compile_settings.optimize ? 1 : 0;
	mono_bool isdll = compile_settings.isDLL ? 1 : 0;
	mono_bool debug = compile_settings.debug ? 1 : 0;
	mono_bool optimize = compile_settings.optimize ? 1 : 0;
	mono_bool verbose = compile_settings.verbose ? 1 : 0;
	args[0] = output;
	args[1] = output_dir;
	args[2] = output_proj;
	args[3] = mono_paths;
	args[4] = mono_refs;
	args[5] = &isdll; // isDLL
	args[6] = &debug; // debug
	args[7] = &optimize; // optimize
	args[8] = &verbose; // verbose


	std::string n = mono_method_full_name(current_method, true);
	std::cout << "Invoking method: " << n << std::endl;
	if (!InvokeCompilerMethod(current_method, args, m_LastCompileFailure))
	{
		m_LastCompileSucceeded = false;
		return;
	}

	MonoMethod* compileMethod = mono_class_get_method_from_name(compilerKlass, "Compile", 0);
	if (!compileMethod)
	{
		std::cout << "Compile method not found!" << std::endl;
		return;
	}
	std::string f = mono_method_full_name(compileMethod, true);
	std::cout << "Invoking method: " << f << std::endl;
	if (!InvokeCompilerMethod(compileMethod, nullptr, m_LastCompileFailure))
	{
		m_LastCompileSucceeded = false;
		return;
	}

	MonoMethod* emitMethod = mono_class_get_method_from_name(compilerKlass, "Emit", 0);
	if (!emitMethod)
	{
		std::cout << "Emit method not found!" << std::endl;
		return;
	}
	std::string g = mono_method_full_name(emitMethod, true);
	std::cout << "Invoking method: " << g << std::endl;
	if (!InvokeCompilerMethod(emitMethod, nullptr, m_LastCompileFailure))
	{
		m_LastCompileSucceeded = false;
		return;
	}
}

void ScriptCompiler::CompileAllScripts()
{
	std::string paths{};



	if (!scripts.empty())
	{
		// Threads
		CompileAsync();
	}
}

void ScriptCompiler::WaitAll()
{
	for (auto& process : processes)
	{
		if (process.joinable())
		{
			process.join();
		}
	}
}
void ScriptCompiler::EndProcesses()
{
	WaitAll();
	processes.clear();
	current_running_thread = 0;
}


void ScriptCompiler::printLog() const
{

	for (const auto& entry : diagnostics)
	{
		std::cout << entry << std::endl;
	}
}

void ScriptCompiler::AddSearchDirectories(std::string const& id, std::string const& path)
{
	search_directories[id].push_back(path);
}

void ScriptCompiler::CollectScripts(std::string const& id)
{
	scripts.clear();
	std::vector<std::string>& dirs = search_directories[id];
	for (const auto& dir : dirs) {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
			if (!entry.is_regular_file() || entry.path().extension() != ".cs") {
				continue;
			}

			const std::filesystem::path path = entry.path();
			const std::string filename = path.filename().string();
			const std::string normalized = path.lexically_normal().generic_string();
			if (normalized.find("/bin/") != std::string::npos ||
				normalized.find("/obj/") != std::string::npos ||
				normalized.find("/managed/") != std::string::npos ||
				EndsWith(filename, ".g.cs") ||
				EndsWith(filename, ".designer.cs")) {
				continue;
			}

			if (entry.is_regular_file() && entry.path().extension() == ".cs") {
				AddScript(entry.path().string());
			}
		}
	}
}

void ScriptCompiler::GetManagedLogs()
{
	MonoDomain* current = mono_domain_get();
	MonoDomain* domain = loader->GetCompilerDomain();
	mono_domain_set(domain, false);
	MonoMethod* getLogsMethod = mono_class_get_method_from_name(compilerKlass, "GetLogs", 0);
	if (!getLogsMethod)
	{
		std::cout << "GetLogs method not found!" << std::endl;
		return;
	}
	std::cout << "Invoking method: " << mono_method_full_name(getLogsMethod, true) << std::endl;
	MonoObject* strList = mono_runtime_invoke(getLogsMethod, nullptr, nullptr, nullptr);

	MonoArray* arr = reinterpret_cast<MonoArray*>(strList);
	if (!arr)
	{
		std::cout << "No logs returned from managed code." << std::endl;
		mono_domain_set(current, false);
		return;
	}
	log.clear();
	diagnostics.clear();
	uint64_t length = mono_array_length(arr);
	for (uint64_t i = 0; i < length; ++i)
	{
		MonoArray* inner = mono_array_get(arr, MonoArray*, i);
		if (!inner)
			continue;
		DiagnosticLog logEntry;
		MonoString* path = mono_array_get(inner, MonoString*, 0);
		MonoString* position = mono_array_get(inner, MonoString*, 1);
		MonoString* severity = mono_array_get(inner, MonoString*, 2);
		MonoString* diagID = mono_array_get(inner, MonoString*, 3);
		MonoString* message = mono_array_get(inner, MonoString*, 4);
		char const * fn = mono_string_to_utf8(path);
		char const* pos = mono_string_to_utf8(position);
		char const* sev = mono_string_to_utf8(severity);
		char const* did = mono_string_to_utf8(diagID);
		char const* msg = mono_string_to_utf8(message);

		logEntry.filename = fn;
		logEntry.position = pos;
		logEntry.severity = sev;
		logEntry.diagnosticID = did;
		logEntry.message = msg;
		diagnostics.push_back(logEntry);
		std::string fullLog = logEntry.filename + "(" + logEntry.position + "): " + logEntry.severity + " " + logEntry.diagnosticID + ": " + logEntry.message;
		log.emplace_back(fullLog);
		
		mono_free((void*)fn);
		mono_free((void*)pos);
		mono_free((void*)sev);
		mono_free((void*)did);
		mono_free((void*)msg);







		/*char* cStr = mono_string_to_utf8(monoStr);
		log.emplace_back(cStr);
		mono_free(cStr);*/
	}

	mono_domain_set(current, false);
}

void ScriptCompiler::AddReferences(std::string const& name, std::string const& path)
{
	assemblies_references[name] = path;
}

void ScriptCompiler::RemoveReferences(std::string const& name)
{
	assemblies_references.erase(name);
}

void ScriptCompiler::ClearReferences()
{
	assemblies_references.clear();
}

void ScriptCompiler::SetCompileOutputName(std::string const& name)
{
	compile_settings.output_name = name;
}
std::string const& ScriptCompiler::GetCompileOutputName() const
{
	return compile_settings.output_name;
}
void ScriptCompiler::SetCompileOutputDirectory(std::string const& dir)
{
	compile_settings.output_directory = dir;
}

void ScriptCompiler::SetProjectOutputDirectory(std::string const& dir)
{
	compile_settings.project_output_dir = dir;
}

std::string const& ScriptCompiler::GetCompileOutputDirectory() const
{
	return compile_settings.output_directory;
}
void ScriptCompiler::SetCompileAsDLL(bool isDLL)
{
	compile_settings.isDLL = isDLL;
}
bool ScriptCompiler::IsCompileAsDLL() const
{
	return compile_settings.isDLL;
}
void ScriptCompiler::SetOptimizeCompile(bool optimize)
{
	compile_settings.optimize = optimize;
}
bool ScriptCompiler::IsOptimizeCompile() const
{
	return compile_settings.optimize;
}
bool ScriptCompiler::IsVerboseCompile() const
{
	return compile_settings.verbose;
}
void ScriptCompiler::SetVerboseCompile(bool verbose)
{
	compile_settings.verbose = verbose;
}

ScriptCompiler::~ScriptCompiler()
{
	EndProcesses();
	compilerAssembly.reset();
}

std::ostream& operator<<(std::ostream& os, const ScriptCompiler::DiagnosticLog& log)
{
	os << log.filename << "(" << log.position << "): " << log.severity << " " << log.diagnosticID << ": " << log.message;
	return os;
}
