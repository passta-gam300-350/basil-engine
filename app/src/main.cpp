#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "Engine.hpp"
#include <glm/glm.hpp>
#include <Render/Render.h>
#include <Manager/ResourceSystem.hpp>
#include <components/transform.h>
#include <filesystem>

int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	ResourceSystem::Instance().ImportAssetList("resource.manifest","assets/bin");
	[[maybe_unused]] auto& rsc = ResourceSystem::Instance();
	if (!std::filesystem::exists("config.yaml")) {
		Engine::GenerateDefaultConfig("config.yaml");
	}
	Engine::InitializeGame();
	Engine::Init("config.yaml");

	Engine::LoadEmbeddedIcon();
	Engine::Update();
	Engine::Exit();
	return 0;
}
int WinMain([[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] LPSTR lpCmdLine, [[maybe_unused]] int nShowCmd) {
	return main();
}