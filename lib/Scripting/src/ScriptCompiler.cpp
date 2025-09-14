#include "ScriptCompiler.hpp"
#include <filesystem>
#include <iostream>
#include <thread>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/class.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/threads.h>


void ScriptCompiler::Init(MonoLoader* ptrLoader, std::string const& compiler_dir)
{
	loader = ptrLoader;
	compiler_path = compiler_dir;

	loader->Enable_Compiler();
	compilerAssembly = loader->LoadAssembly(compiler_path);
	compilerImage = loader->LoadImage(compilerAssembly);

	compilerKlass = mono_class_from_name(compilerImage, "", "CSCompiler");
	/*
	 *public static void Setup(string output, string outputDirectory, string[] sources, string[] references, bool isDLL = false, bool debug = false,
	 *bool optimize = false, bool verbose = false)
	 */
	current_method = mono_class_get_method_from_name(compilerKlass, "Setup", 8);
	current_running_thread = 0;

}



std::vector<std::string> const& ScriptCompiler::GetLogs() const
{
	return log;
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
	void* args[8];
	/*
	 *public static void Setup(string output, string outputDirectory, string[] sources, string[] references, bool isDLL = false, bool debug = false,
	 *bool optimize = false, bool verbose = false)
	*/
	MonoString* output = mono_string_new(compiler_d, "UserApplication");
	MonoString* output_dir = mono_string_new(compiler_d, "bin");
	mono_bool val = false;
	mono_bool isdll = false;
	mono_bool debug = false;
	args[0] = output;
	args[1] = output_dir;
	args[2] = mono_paths;
	args[3] = mono_refs;
	args[4] = &isdll; // isDLL
	args[5] = &val; // debug
	args[6] = &val; // optimize
	args[7] = &val; // verbose


	std::string n = mono_method_full_name(current_method, true);
	std::cout << "Invoking method: " << n << std::endl;
	mono_runtime_invoke(current_method, nullptr, args, nullptr);

	MonoMethod* compileMethod = mono_class_get_method_from_name(compilerKlass, "Compile", 0);
	if (!compileMethod)
	{
		std::cout << "Compile method not found!" << std::endl;
		return;
	}
	std::string f = mono_method_full_name(compileMethod, true);
	std::cout << "Invoking method: " << f << std::endl;
	mono_runtime_invoke(compileMethod, nullptr, nullptr, nullptr);

	MonoMethod* emitMethod = mono_class_get_method_from_name(compilerKlass, "Emit", 0);
	if (!emitMethod)
	{
		std::cout << "Emit method not found!" << std::endl;
		return;
	}
	std::string g = mono_method_full_name(emitMethod, true);
	std::cout << "Invoking method: " << g << std::endl;
	mono_runtime_invoke(emitMethod, nullptr, nullptr, nullptr);
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
	
	for (const auto& entry : log)
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
	int length = mono_array_length(arr);
	for (int i = 0; i < length; ++i)
	{
		MonoString* monoStr = mono_array_get(arr, MonoString*, i);
		char* cStr = mono_string_to_utf8(monoStr);
		log.emplace_back(cStr);
		mono_free(cStr);
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
