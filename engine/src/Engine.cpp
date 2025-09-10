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
#ifdef _DEBUG
	spdlog::info("Engine Update");
#endif
}


void Engine::FixedUpdate()
{
#ifdef _DEBUG
	spdlog::info("Engine Fixed Update");
#endif
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



