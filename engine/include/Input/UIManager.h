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
#include <vector>
#include <memory>
#include "Button.h"
#include "InputManager.h"

class Button;

class UIManager
{
    public:
        static UIManager* Get_Instance();
        static void Destroy_Instance();

        // Life cycle
        void OpenUI();
        void CloseUI();
        bool IsUIOpen() const;

        void Update();
        void Render();

        // Elements
        void AddButton(std::shared_ptr<Button> button);
        void Clear();

    private:
        UIManager() = default;

        static UIManager* instance;

        bool uiOpen = false;
        std::vector<std::shared_ptr<Button>> buttons;
};
