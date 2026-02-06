#include "Bindings/ManagedScene.hpp"

#include "Engine.hpp"
#include "mono_include_pkg.h"
#include "Scene/Scene.hpp"
void ManagedScene::LoadSceneIndex(int32_t index)
{
	// Implementation for loading a scene by its index
	// This is a placeholder implementation
	// Actual implementation would interact with the engine's scene management system

	auto& Engine = ::Engine::Instance();

	Engine.GetSceneRegistry().RequestSceneChange(index);
}
void ManagedScene::LoadSceneName(MonoString* name)
{
	// Implementation for loading a scene by its name
	// This is a placeholder implementation
	// Actual implementation would convert MonoString to std::string
	// and interact with the engine's scene management system

	auto& Engine = ::Engine::Instance();
	char* name_cstr = mono_string_to_utf8(name);

	std::string path = std::string{ Engine.getWorkingDir() } + "/" + std::string{ name_cstr };
	Engine.GetSceneRegistry().RequestSceneChange(path);

	mono_free(name_cstr);
}

void ManagedScene::ExitApplication()
{
	// TODO: Implementation for exiting the application
}
