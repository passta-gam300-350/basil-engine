#ifndef Engine_HPP
#define Engine_HPP
#include <string>

class Renderer;
class Window;

class Engine
{
public:

	static Window* m_Window;
	static Renderer* m_Renderer;
	
	static void Init(std::string const& name, unsigned width, unsigned height);
	static void Input();
	static void Update();
	static void FixedUpdate();
	static void Exit();


	static bool WindowShouldClose();
};
#endif // Engine_HPP