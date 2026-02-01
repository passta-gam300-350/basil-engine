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
}

void UIManager::Destroy_Instance()
{
}

void UIManager::OpenUI()
{
}

void UIManager::CloseUI()
{
}

bool UIManager::IsUIOpen() const
{
}

void UIManager::AddButton(std::shared_ptr<Button> button)
{
}

void UIManager::Clear()
{
}

void UIManager::Update()
{
}

void UIManager::Render()
{
}
