#ifndef EDITORMAIN_HPP
#define EDITORMAIN_HPP

#include "imgui.h"
#include "Screens/Screen.hpp"

class EditorMain : public Screen
{
public:
	bool showAboutModal = false;




	EditorMain(GLFWwindow* window);
	void init() override;
	void render() override;
	void cleanup() override;
	void Show() override;
	void Close() override;
	virtual bool isWindowClosed() override;
	bool Activate() override;





	~EditorMain() override = default;



	void Setup_Dockspace(unsigned id);

	void Render_MenuBar();

	void Render_AboutUI();


	void Render_SceneExplorer();



	void Render_Console();



	void Render_Scene();
	void Render_Game();

private:
	// Entity management
	void CreateDefaultEntity();
	void CreatePlaneEntity();
	void CreateLightEntity();
	void CreateCameraEntity();

	// Camera controls (following GraphicsTestDriver pattern)
	void ProcessCameraInput();

	// Mouse camera controls
	bool m_CameraControlEnabled = false;
	bool m_FirstMouse = true;
	float m_LastMouseX = 640.0f;
	float m_LastMouseY = 360.0f;
	float m_MouseSensitivity = 0.1f;
};
#endif // EDITORMAIN_HPP