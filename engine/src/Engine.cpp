#include "Engine.hpp"
#include "Core/Window.h"
#include "Core/Renderer.h"
#include "spdlog/spdlog.h"
#include "Scene/Scene.hpp"
#include "Manager/SceneManager.hpp"
#include "Manager/StringManager.hpp"
#include "Render/Render.h"

#include <yaml-cpp/yaml.h>

std::unique_ptr<Window> Engine::m_Window{};

void Engine::Init(std::string const& name, unsigned width, unsigned height, std::string const& cfg)
{
	ecs::init_ecs();
	m_Window.reset(new Window{ name, width, height });
	RenderSystem::NewInstance();
	RenderSystem::Instance().Init();

	if (cfg.empty()) {
		ResourceSystem::Init();
	}
	else {
		YAML::Node node{ YAML::Load(cfg) };
		if (node["resource_file_list"]) {
			ResourceSystem::Init(node["resource_file_list"].as<std::string>());
		}
	}

	spdlog::info("Started Engine.");
	spdlog::info("Engine Name: {}, Width: {}, Height: {}" ,name,width, height);

	Scene newScene = SceneManager::GetInstance().CreateEmpty("Hello");
	spdlog::info("Created Scene: {}", StringManager::GetInstance().Get_String(newScene.GetMetadata().name));

}

void Engine::Input()
{
	m_Window->PollEvents();
}

void Engine::Update()
{
	//RenderSystem::Instance().Update();
}


void Engine::FixedUpdate()
{

}


void Engine::Exit()
{
#ifdef _DEBUG
	spdlog::info("Engine Exit");
#endif
}

Window& Engine::GetWindowInstance()
{
	return *m_Window;
}


bool Engine::WindowShouldClose()
{
	return m_Window->ShouldClose();
}



