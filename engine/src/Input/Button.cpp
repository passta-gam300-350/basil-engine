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
#include "Input/InputManager.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <utility>

namespace
{
    std::pair<float, float> GetAnchorFactors(Button::Anchor anchor)
    {
        switch (anchor)
        {
        // Match HUD shader anchor semantics exactly.
        case Button::Anchor::TopLeft: return { 0.0f, 1.0f };
        case Button::Anchor::TopCenter: return { 0.5f, 1.0f };
        case Button::Anchor::TopRight: return { 1.0f, 1.0f };
        case Button::Anchor::CenterLeft: return { 0.0f, 0.5f };
        case Button::Anchor::Center: return { 0.5f, 0.5f };
        case Button::Anchor::CenterRight: return { 1.0f, 0.5f };
        case Button::Anchor::BottomLeft: return { 0.0f, 0.0f };
        case Button::Anchor::BottomCenter: return { 0.5f, 0.0f };
        case Button::Anchor::BottomRight: return { 1.0f, 0.0f };
        }

        return { 0.0f, 0.0f };
    }
}

Button::Button() : x(),y(),width(),height(),anchor(Anchor::TopLeft),hovered(),pressed(),clicked(),disabled()
{
	
}

Button::Button(float x, float y, float width, float height, const std::string& text)
    : x(x),
      y(y),
      width(width),
      height(height),
      anchor(Anchor::TopLeft),
      text(text),
      hovered(false),
      pressed(false),
      clicked(false),
      disabled(false)
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
    clicked = false;

    if (disabled)
    {
        hovered = false;
        pressed = false;
        return;
    }

    auto [anchorX, anchorY] = GetAnchorFactors(anchor);
    const float left = x - (width * anchorX);
    const float top = y - (height * anchorY);
    const float right = left + width;
    const float bottom = top + height;

    // AABB hit test
    this->hovered = mouseX >= left && mouseX <= right && mouseY >= top && mouseY <= bottom;

    // Fire once on press
    if (this->hovered && mousePressed && !this->pressed && !input->Is_MouseConsumed())
    {
        this->pressed = true;
        this->clicked = true;

        // UI consumes the mouse so gameplay won't see
        input->Consume_Mouse();
        if (this->onClick)
            onClick();
    }

    // Reset press state on release
    if (input->Is_MouseReleased(GLFW_MOUSE_BUTTON_LEFT) || !mousePressed)
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
