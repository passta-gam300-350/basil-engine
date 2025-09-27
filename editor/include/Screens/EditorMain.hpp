#ifndef EDITORMAIN_HPP
#define EDITORMAIN_HPP

#include "imgui.h"
#include "Screens/Screen.hpp"
#include "Camera/EditorCamera.hpp"
#include <memory>

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
	void Render_CameraControls();

private:
	// Entity management
	void CreateDefaultEntity();
	void CreatePlaneEntity();
	void CreateLightEntity();
	void CreateCameraEntity();

	// Editor Camera
	std::unique_ptr<EditorCamera> m_EditorCamera;

	// Play mode state
	bool m_IsPlayMode = false;

	// Viewport size tracking for aspect ratio
	float m_ViewportWidth = 1280.0f;
	float m_ViewportHeight = 720.0f;
};
#endif // EDITORMAIN_HPP