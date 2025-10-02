#ifndef SPLASHSCREEN_HPP
#define SPLASHSCREEN_HPP


#include "Screen.hpp"

class SplashScreen : public Screen
{
	float timer = 1.5f; // 5 seconds
	
public:
	SplashScreen(GLFWwindow* window);
	void init() override;
	void update() override;
	void render() override;
	void cleanup() override;

	void Show() override;
	void Close() override;

	virtual bool isWindowClosed() override;
	bool Activate() override;

	~SplashScreen() override = default;

};

#endif // SPLASHSCREEN_HPP