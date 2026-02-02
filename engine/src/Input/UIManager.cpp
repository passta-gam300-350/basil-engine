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
#include "UIManager.h"

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

bool UIManager::IsUIOpen() const
{
    return uiOpen;
}

void UIManager::AddButton(std::shared_ptr<Button> button)
{
    buttons.push_back(button);
}

void UIManager::Clear()
{
    buttons.clear();
}

void UIManager::Update()
{
    if (!uiOpen)
        return;

    for (auto& button : buttons)
        button->update();
}

void UIManager::Render()
{
    if (!uiOpen)
        return;

    for (auto& button : buttons)
    {
        button->render();
    }
}
