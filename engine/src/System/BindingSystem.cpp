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