#include "System/ButtonSystem.hpp"
#include "Render/Camera.h"
#include "Render/Render.h"
#include <Rendering/HUDRenderer.h>
#include "Input/Button.h"
#include "Input/InputManager.h"
#include "Profiler/profiler.hpp"

namespace
{
    bool IsValidTextureGuid(const rp::BasicIndexedGuid& guid)
    {
        return guid.m_guid != rp::null_guid;
    }

    const rp::BasicIndexedGuid* ResolveButtonTexture(const Button& button)
    {
        if (button.pressed && IsValidTextureGuid(button.pressedTextureGuid))
        {
            return &button.pressedTextureGuid;
        }

        if (button.hovered && IsValidTextureGuid(button.hoverTextureGuid))
        {
            return &button.hoverTextureGuid;
        }

        if (IsValidTextureGuid(button.defaultTextureGuid))
        {
            return &button.defaultTextureGuid;
        }

        return nullptr;
    }
}

ButtonSystem& ButtonSystem::Instance()
{
    static ButtonSystem instance;
    return instance;
}

void ButtonSystem::Init()
{
}

void ButtonSystem::Update(ecs::world& world, float)
{
    PF_SYSTEM("Button System");

    InputManager* input = InputManager::Get_Instance();
    if (!input)
    {
        return;
    }

    float mouseX = 0.0f;
    float mouseY = 0.0f;
    input->Get_MousePosition(mouseX, mouseY);
    const bool mousePressed = input->Is_MousePressed(GLFW_MOUSE_BUTTON_LEFT);
    glm::vec2 viewportOffset = CameraSystem::GetCachedViewportOffset();
    glm::vec2 viewportSize = CameraSystem::GetCachedViewport();
    glm::vec2 hudReferenceResolution{ 1920.0f, 1080.0f };

    if (auto* sceneRenderer = Engine::GetRenderSystem().GetSceneRenderer())
    {
        if (auto* hudRenderer = sceneRenderer->GetHUDRenderer())
        {
            hudReferenceResolution = hudRenderer->GetReferenceResolution();
        }
    }

    for (auto entity : world.filter_entities<Button>())
    {
        Button& button = world.get_component_from_entity<Button>(entity);

        if (button.disabled)
        {
            button.hovered = false;
            button.pressed = false;
            button.clicked = false;
            continue;
        }

        if (entity.all<HUDComponent>())
        {
            HUDComponent& hud = entity.get<HUDComponent>();
            button.x = hud.position.x;
            button.y = hud.position.y;
            button.width = hud.size.x;
            button.height = hud.size.y;
            button.anchor = static_cast<Button::Anchor>(hud.anchor);

            if (!hud.visible)
            {
                button.hovered = false;
                button.pressed = false;
                button.clicked = false;
                continue;
            }

            float buttonMouseX = mouseX;
            float buttonMouseY = mouseY;
            if (viewportSize.x > 0.0f && viewportSize.y > 0.0f)
            {
                const float localX = mouseX - viewportOffset.x;
                const float localY = mouseY - viewportOffset.y;
                buttonMouseX = (localX / viewportSize.x) * hudReferenceResolution.x;
                buttonMouseY = hudReferenceResolution.y - ((localY / viewportSize.y) * hudReferenceResolution.y);
            }

            button.update(buttonMouseX, buttonMouseY, mousePressed);
        }
        else
        {
            button.update(mouseX, mouseY, mousePressed);
        }

        if (entity.all<HUDComponent>())
        {
            HUDComponent& hud = entity.get<HUDComponent>();
            if (const rp::BasicIndexedGuid* textureGuid = ResolveButtonTexture(button))
            {
                hud.m_TextureGuid = *textureGuid;
            }
        }
    }
}

void ButtonSystem::FixedUpdate(ecs::world& world)
{
    Update(world, 0.0f);
}

void ButtonSystem::Exit()
{
}
