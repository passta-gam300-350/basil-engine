/******************************************************************************/
/*!
\file   BindingSystem.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Binding system implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "System/BindingSystem.hpp"

#include "../CPPGlue/EngineBindingGlue_Includes.hpp"
#include <spdlog/spdlog.h>

void BindingSystem::RegisterBindings() {
	// Register all generated bindings
	try {
		#include "../CPPGlue/EngineBindingGlue.def"
	}
	catch (const std::exception& e) {
		spdlog::error("[BindingSystem] Exception during binding registration: {}", e.what());
	}
}