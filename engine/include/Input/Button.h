/*!************************************************************************
\file:      Button.h
\author:    Saminathan Aaron Nicholas
\email:     s.aaronnicholas@digipen.edu
\course:    CSD 3451 - Software Engineering Project 6
\brief:     This file implements the class for the button component.

            Methods are also offered to query key and mouse states, fetching mouse positions, and retrieving scroll offsets.
            It also resets input states at the end of each frame and handles input state persistence between frames.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
**************************************************************************/
#pragma once
#include <functional>
#include <string>

class Button
{
    public:
        Button(float x, float y, float width, float height, const std::string& text);

        void setOnClick(std::function<void()> callback);

        void update(float mouseX, float mouseY, bool mousePressed);
        void render() const;

        bool isHovered() const;

    private:
        float x, y;
        float width, height;
        std::string text;

        bool hovered;
        bool pressed;

        std::function<void()> onClick;
};