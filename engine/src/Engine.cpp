#include "Engine.hpp"
#include "Core/Window.h"
#include "Core/Renderer.h"
#include "spdlog/spdlog.h"
#include "Scene/Scene.hpp"
#include "Manager/SceneManager.hpp"
#include "Manager/StringManager.hpp"

Window* Engine::m_Window = nullptr;
Renderer* Engine::m_Renderer = nullptr;

void Engine::Init(std::string const& name, unsigned width, unsigned height)
{
	ecs::init_ecs();
	m_Window = new Window(name, width, height);
	m_Renderer = new Renderer();
	m_Renderer->Initialize(m_Window->GetNativeWindow());


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


bool Engine::WindowShouldClose()
{
	return m_Window->ShouldClose();
}



