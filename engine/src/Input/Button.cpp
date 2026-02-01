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
#include "Button.h"
#include "InputManager.h"
#include <GLFW/glfw3.h>
#include <iostream>

Button::Button(float x, float y, float width, float height, const std::string& text)
    : x(x),
      y(y),
      width(width),
      height(height),
      text(text),
      hovered(false),
      pressed(false)
{}

void Button::setOnClick(std::function<void()> callback)
{
    this->onClick = std::move(callback);
}

bool Button::isHovered() const
{
    return this->hovered;
}

void Button::update(float mouseX, float mouseY, bool mousePressed)
{
    InputManager* input = InputManager::Get_Instance();

    float mx, my;
    input->Get_MousePosition(mx, my);

    // AABB hit test
    this->hovered = mx >= x && mx <= x + width && my >= y && my <= y + height;

    // Fire once on press
    if (this->hovered && input->Is_MousePressed(GLFW_MOUSE_BUTTON_LEFT) && !this->pressed)
    {
        this->pressed = true;

        // UI consumes the mouse so gameplay won't see
        input->Consume_Mouse();
        if (this->onClick)
            onClick();
    }

    // Reset press state on release
    if (input->Is_MouseReleased(GLFW_MOUSE_BUTTON_LEFT))
        this->pressed = false;
}

void Button::render() const
{
    // Replace this with how button is to be rendered
    std::cout << "[Button]: " << this->text << ", (hovered): " << this->hovered << "\n";
}

/*
Button playButton(100, 100, 200, 50, "Play");
playButton.setOnClick([]()
    {
        std::cout << "Play button clicked!\n";
    });

In game loop:
    playButton.update(mouseX, mouseY, mousePressed);
    playButton.render();
*/
