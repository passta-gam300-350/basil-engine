#include <iostream>

// Mono test main
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>

#include "MonoManager.hpp"
#include "ScriptCompiler.hpp"

int main(int argc, char* argv[]) {
	MonoManager::Initialize();
	MonoManager::SetVerbose(true);
	MonoManager::AddSearchDirectories("../../../../../scripts");
	
	MonoManager::GetCompiler()->AddReferences("engine",R"(C:\Users\yeo_j\Documents\Digipen Repo\Year 3\Project\Project\engine\managed\EngineAPI\bin\EngineAPI.dll)");
	MonoManager::StartCompilation();

	auto a  = MonoManager::GetCompiler()->GetLogs();

	for (const auto& entry : a) {
		std::cout << entry << std::endl;
	}
}
