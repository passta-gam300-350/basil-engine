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

void Button::setOnClick(Callback callback)
{
}

bool Button::isHovered() const
{
}

void Button::update(float mouseX, float mouseY, bool mousePressed)
{
}

void Button::render() const
{
}
