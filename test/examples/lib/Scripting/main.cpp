/******************************************************************************/
/*!
\file   main.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Scripting test driver

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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
	MonoManager::AddSearchDirectories("../scripts");
	
	MonoManager::GetCompiler()->AddReferences("engine",R"(..\engine\managed\BasilEngine\bin\BasilEngine.dll)");
	MonoManager::StartCompilation();

	auto a  = MonoManager::GetCompiler()->GetLogs();

	for (const auto& entry : a) {
		std::cout << entry << std::endl;
	}
}
