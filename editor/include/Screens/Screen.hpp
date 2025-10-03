#ifndef SCREEN_HPP
#define SCREEN_HPP
#include <cstdint>

// A window or screen in the editor
struct GLFWwindow;
class Screen {
protected:

	GLFWwindow* window; // Associated window
	int width, height; // Dimensions of the window
	int posx, posy; // Position of the window

	uint32_t WIN_DECORATOR; // Window decorator character

	bool active = true; // Is the screen active and should be rendered?

public:
	Screen() = default;
	Screen(GLFWwindow* window) { this->window = window; }
	virtual ~Screen() = default;
	virtual void init() = 0;
	virtual void update() = 0;
	virtual void render() = 0;
	virtual void cleanup() = 0;
	virtual void Show();
	virtual void Close();


	virtual bool isWindowClosed() = 0;


	virtual bool Activate() = 0;


	



};

#endif // SCREEN_HPP