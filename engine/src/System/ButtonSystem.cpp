#include "System/ButtonSystem.hpp"
#include "Render/Render.h"
#include "Input/Button.h"
#include "Input/InputManager.h"
#include "Profiler/profiler.hpp"


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

    for (auto entity : world.filter_entities<Button>())
    {
        Button& button = world.get_component_from_entity<Button>(entity);

        if (entity.all<HUDComponent>())
        {
            const HUDComponent& hud = entity.get<HUDComponent>();
            button.x = hud.position.x;
            button.y = hud.position.y;
            button.width = hud.size.x;
            button.height = hud.size.y;
            button.anchor = static_cast<Button::Anchor>(hud.anchor);

            if (!hud.visible)
            {
                button.hovered = false;
                button.pressed = false;
                continue;
            }
        }

        button.update(mouseX, mouseY, mousePressed);
    }
}

void ButtonSystem::FixedUpdate(ecs::world& world)
{
    Update(world, 0.0f);
}

void ButtonSystem::Exit()
{
}
