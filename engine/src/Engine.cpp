#include "Engine.hpp"
#include "Core/Window.h"
#include "Core/Renderer.h"
#include "spdlog/spdlog.h"

Window* Engine::m_Window = nullptr;
Renderer* Engine::m_Renderer = nullptr;

void Engine::Init(std::string const& name, unsigned width, unsigned height)
{
	m_Window = new Window(name, width, height);
	m_Renderer = new Renderer();
	m_Renderer->Initialize(m_Window->GetNativeWindow());

}

void Engine::Input()
{
	m_Window->PollEvents();
}

void Engine::Update()
{

	spdlog::info("Engine Update");
}


void Engine::FixedUpdate()
{

	spdlog::info("Engine Fixed Update");
}


void Engine::Exit()
{

	spdlog::info("Engine Exit");
}


bool Engine::WindowShouldClose()
{
	return m_Window->ShouldClose();
}



