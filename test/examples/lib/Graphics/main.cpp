// test/examples/lib/graphics/main.cpp
#include "Application.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

class GraphicsTestApp : public Application
{
public:
    GraphicsTestApp() : Application("Graphics Engine Test", 1280, 720)
    {
    }

    void LoadResources() override
    {
        std::cout << "Loading resources..." << std::endl;

        // Load shader using clean graphics lib interface
        LoadShader("basic", 
            "assets/shaders/basic.vert", 
            "assets/shaders/basic.frag");

        // Load model using clean graphics lib interface  
        LoadModel("tinbox", "assets/models/tinbox/tin_box.obj");

        // Set up camera position
        SetCameraPosition(glm::vec3(0.0f, 0.0f, 3.0f));
        
        std::cout << "Resources loaded successfully!" << std::endl;
    }

    void Update(float deltaTime) override
    {
        // Rotate the model over time
        m_ModelRotation += deltaTime * 50.0f; // 50 degrees per second
        
        // Optional: Move camera in a circle around the model
        if (m_CircleCamera) {
            float radius = 5.0f;
            float x = sin(deltaTime * m_CameraSpeed) * radius;
            float z = cos(deltaTime * m_CameraSpeed) * radius;
            SetCameraPosition(glm::vec3(x, 0.0f, z));
        }
    }

    void OnKeyboard(int key, int action, float deltaTime) override
    {
        // Handle additional key inputs
        if (action == GLFW_PRESS) {
            switch (key) {
                case GLFW_KEY_R:
                    // Reset camera position
                    SetCameraPosition(glm::vec3(0.0f, 0.0f, 3.0f));
                    SetCameraRotation(glm::vec3(0.0f, 0.0f, 0.0f));
                    std::cout << "Camera reset" << std::endl;
                    break;
                case GLFW_KEY_C:
                    // Toggle camera circle mode
                    m_CircleCamera = !m_CircleCamera;
                    std::cout << "Circle camera: " << (m_CircleCamera ? "ON" : "OFF") << std::endl;
                    break;
                case GLFW_KEY_SPACE:
                    // Toggle model rotation
                    m_RotateModel = !m_RotateModel;
                    std::cout << "Model rotation: " << (m_RotateModel ? "ON" : "OFF") << std::endl;
                    break;
            }
        }
    }

    void OnMouseScroll(double yoffset) override
    {
        // Additional scroll handling if needed
        // std::cout << "Scroll: " << yoffset << std::endl;
    }

    void Render() override
    {
        // Render the model using clean graphics lib interface
        
        // Create transformation matrix
        glm::mat4 model = glm::mat4(1.0f);
        
        if (m_RotateModel) {
            model = glm::rotate(model, glm::radians(m_ModelRotation), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        
        // Scale down the model a bit
        model = glm::scale(model, glm::vec3(0.5f));
        
        // Render using graphics lib - handles all OpenGL calls internally
        RenderModel("tinbox", "basic", model);
    }

private:
    float m_ModelRotation = 0.0f;
    bool m_RotateModel = true;
    bool m_CircleCamera = false;
    float m_CameraSpeed = 0.5f;
};

int main()
{
    try
    {
        std::cout << "=== Starting Graphics Engine Test ===" << std::endl;
        
        // Create and run application - graphics lib handles everything internally
        GraphicsTestApp app;
        app.Run();
        
        std::cout << "=== Graphics Engine Test Complete ===" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Application error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}