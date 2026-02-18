/*!************************************************************************
\file:      UIManager.h
\author:    Saminathan Aaron Nicholas
\email:     s.aaronnicholas@digipen.edu
\course:    CSD 3451 - Software Engineering Project 6
\brief:     This file implements the class used for UI which owns buttons and actions of the UI.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
**************************************************************************/

#pragma once
#include "ecs/ecs.h"

class UIManager
{
public:
    static UIManager* Get_Instance();
    static void Destroy_Instance();

    void OpenUI();
    void CloseUI();
    bool IsUIOpen() const noexcept;

    // Runs UI interaction
    void Update(ecs::world w);

private:
    UIManager() = default;

    static UIManager* instance;
    bool uiOpen = false;
};
