/*!************************************************************************
\file:      Button.cpp
\author:    Saminathan Aaron Nicholas
\email:     s.aaronnicholas@digipen.edu
\course:    CSD 3451 - Software Engineering Project 6
\brief:     This file implements the class of the button and determines if the button is being hovered over, clicked or
            not interacted with.

            Methods are also offered to query key and mouse states, fetching mouse positions, and retrieving scroll offsets.
            It also resets input states at the end of each frame and handles input state persistence between frames.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
**************************************************************************/
#include "Button.h"
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
    this->onClick = callback;
}

bool Button::isHovered() const
{
    return this->hovered;
}

void Button::update(float mouseX, float mouseY, bool mousePressed)
{
    this->hovered = mouseX >= x && mouseX <= x + width && mouseY >= y && mouseY <= y + height;

    if (this->hovered && mousePressed && !this->pressed)
    {
        this->pressed = true;
        if (this->onClick)
            onClick();
    }

    if (!mousePressed)
        this->pressed = false;
}

void Button::render() const
{
    // Replace this with how button is to be rendered
    std::cout << "[Button]: " << this->text << ", (hovered): " << this->hovered << "\n";
}
