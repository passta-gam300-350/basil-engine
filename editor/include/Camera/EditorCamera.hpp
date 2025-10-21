#ifndef EDITOR_CAMERA_HPP
#define EDITOR_CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <Render/Camera.h>

class EditorCamera : public CameraSystem::Camera
{
public:
    enum class Mode
    {
        Orbit,    // Orbit around target
        Fly,      // FPS-style fly camera
        Pan       // 2D panning
    };

    EditorCamera();
    ~EditorCamera() = default;

    // Update camera based on input
    void Update(float deltaTime);

    // Camera manipulation
    void SetPosition(const glm::vec3& position) { m_Pos = position; }
    void SetRotation(const glm::vec3& rotation) { m_Rotation = rotation; }
    void SetTarget(const glm::vec3& target) { m_Target = target; }
    void SetMode(Mode mode) { m_Mode = mode; }

    // Camera properties
    void SetFOV(float fov) { m_Fov = fov; UpdateProjection(); }
    void SetAspectRatio(float aspect) { m_AspectRatio = aspect; UpdateProjection(); }
    void SetNearPlane(float nearPlane) { m_Near = nearPlane; UpdateProjection(); }
    void SetFarPlane(float farPlane) { m_Far = farPlane; UpdateProjection(); }

    // Getters
    glm::vec3 GetPosition() const { return m_Pos; }
    glm::vec3 GetRotation() const { return m_Rotation; }
    glm::vec3 GetTarget() const { return m_Target; }
    glm::vec3 GetForward() const { return m_Front; }
    glm::vec3 GetRight() const { return m_Right; }
    glm::vec3 GetUp() const { return m_Up; }

    // Matrix getters
    const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
    const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
    const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

    // Input handling
    void OnMouseScroll(float delta);
    void OnMouseMove(float deltaX, float deltaY);

    // Control settings
    void SetMoveSpeed(float speed) { m_MoveSpeed = speed; }
    void SetRotationSpeed(float speed) { m_RotationSpeed = speed; }
    void SetZoomSpeed(float speed) { m_ZoomSpeed = speed; }

    float GetMoveSpeed() const { return m_MoveSpeed; }
    float GetRotationSpeed() const { return m_RotationSpeed; }

    // Focus on object
    void FocusOn(const glm::vec3& target, float distance = 5.0f);

    // Reset camera
    void Reset();

private:
    void UpdateVectors();
    void UpdateView();
    void UpdateProjection();

    void HandleKeyboardInput(float deltaTime);
    void HandleMouseInput(float deltaTime);

    // Fly mode controls
    void UpdateFlyMode(float deltaTime);

    // Orbit mode controls
    void UpdateOrbitMode(float deltaTime);

    // Pan mode controls
    void UpdatePanMode(float deltaTime);

private:
    // Camera mode
    Mode m_Mode = Mode::Fly;

    // Transform
    glm::vec3 m_Position{0.0f, 5.0f, 10.0f};
    glm::vec3 m_Rotation{0.0f, 0.0f, 0.0f}; // Pitch, Yaw, Roll
    glm::vec3 m_Target{0.0f, 0.0f, 0.0f};

    // Matrices
    glm::mat4 m_ViewMatrix{1.0f};
    glm::mat4 m_ProjectionMatrix{1.0f};
    glm::mat4 m_ViewProjectionMatrix{1.0f};

    // Control settings
    float m_MoveSpeed = 5.0f;
    float m_RotationSpeed = 0.3f;
    float m_ZoomSpeed = 2.0f;
    float m_PanSpeed = 0.01f;
    float m_OrbitDistance = 10.0f;

    // Input state tracking
    glm::vec2 m_LastMousePosition{0.0f};
    bool m_FirstMouse = true;
    bool m_IsRightMousePressed = false;
    bool m_IsMiddleMousePressed = false;
    bool m_IsLeftMousePressed = false;

    // Smoothing
    glm::vec3 m_VelocitySmoothing{0.0f};
    glm::vec2 m_RotationSmoothing{0.0f};
};

#endif // EDITOR_CAMERA_HPP