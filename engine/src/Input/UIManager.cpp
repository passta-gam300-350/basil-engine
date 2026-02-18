/*!************************************************************************
\file:      UIManager.cpp
\author:    Saminathan Aaron Nicholas
\email:     s.aaronnicholas@digipen.edu
\course:    CSD 3451 - Software Engineering Project 6
\brief:     This file implements the class for the UI Manager which is responsible for the UI and buttons used
            in the UI. This includes the switching of contexts used in the Input Manager to determine if
            input should be in Gameplay mode or UI mode.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
**************************************************************************/

#include "Input/UIManager.h"
#include "Input/InputManager.h"
#include "Input/Button.h"
#include <GLFW/glfw3.h>

UIManager* UIManager::instance = nullptr;

UIManager* UIManager::Get_Instance()
{
    if (!instance)
        instance = new UIManager();
    return instance;
}

void UIManager::Destroy_Instance()
{
    delete instance;
    instance = nullptr;
}

void UIManager::OpenUI()
{
    uiOpen = true;
    InputManager::Get_Instance()->SetInputContext(InputManager::InputContext::UI);
}

void UIManager::CloseUI()
{
    uiOpen = false;
    InputManager::Get_Instance()->SetInputContext(InputManager::InputContext::Gameplay);
}

bool UIManager::IsUIOpen() const noexcept
{
    return uiOpen;
}

void UIManager::Update(ecs::world w)
{
    if (!uiOpen)
        return;

    auto& reg = w.impl.get_registry();
    auto view = reg.view<Button>();

    InputManager* input = InputManager::Get_Instance();

    float mx{}, my{};
    input->Get_MousePosition(mx, my);

    for (auto entity : view)
    {
        auto& btn = view.get<Button>(entity);

        btn.hovered =
            mx >= btn.x && mx <= btn.x + btn.width &&
            my >= btn.y && my <= btn.y + btn.height;

        if (btn.hovered &&
            input->Is_MousePressed(GLFW_MOUSE_BUTTON_LEFT) &&
            !btn.pressed)
        {
            btn.pressed = true;
            input->Consume_Mouse();

            if (btn.onClick)
                btn.onClick();
        }

        if (input->Is_MouseReleased(GLFW_MOUSE_BUTTON_LEFT))
            btn.pressed = false;
    }
}
