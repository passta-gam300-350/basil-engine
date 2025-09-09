#include <GLFW/glfw3.h>

#include "Engine.hpp"
int main() {
	Engine::Init("Name", 2400, 1080);
	while (!Engine::WindowShouldClose()) {
		Engine::Input();
		Engine::Update();
		
	}
	Engine::Exit();
}