/*!************************************************************************
\file:      Button.cpp
\author:    Saminathan Aaron Nicholas
\email:     s.aaronnicholas@digipen.edu
\course:    CSD 3451 - Software Engineering Project 6
\brief:     This file implements the class of the button and determines if the button is being hovered over, clicked or
            not interacted with.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
**************************************************************************/

#include "Input/Button.h"

Button::Button(float x, float y, float width, float height, const std::string& text)
   : x(x),
    y(y),
    width(width),
    height(height),
    text(text),
    hovered(false),
    pressed(false)
{
}

void Button::setOnClick(std::function<void()> callback)
{
    onClick = std::move(callback);
}

bool Button::isHovered() const noexcept
{
    return hovered;
}

/*
    Editor creating buttons:
        ecs::entity e = world.add_entity();
        auto& btn = e.add<Button>();

        btn = Button(100.f, 100.f, 200.f, 50.f, "Play");
        btn.setOnClick([]()
        {
            std::cout << "Play button clicked\n";
        });

    Setting on click:
        btn.setOnClick([]()
        {
            Engine::LoadScene("Level1.scene");
        });
*/