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
	ResourceSystem::Instance().ImportAssetList(".assetlist","assets/bin");
	auto& rsc = ResourceSystem::Instance();
	Engine::Init("Default.yaml");
	Engine::Update();
	Engine::Exit();
	return 0;
}