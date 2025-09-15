#ifndef Engine_HPP
#define Engine_HPP
#include <string>
#include <memory>
#include "Ecs/ecs.h"

class SceneRenderer;
class Window;

class Engine
{
public:

	static std::unique_ptr<Window> m_Window;
	static ecs::world m_World;
	
	static void Init(std::string const& name, unsigned width, unsigned height, std::string const& cfg = {});
	static void Input();
	static void Update();
	static void FixedUpdate();
	static void Exit();

	static Window& GetWindowInstance();


	static bool WindowShouldClose();
};
#endif // Engine_HPP