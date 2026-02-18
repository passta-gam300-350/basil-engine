/*!************************************************************************
\file:      Button.h
\author:    Saminathan Aaron Nicholas
\email:     s.aaronnicholas@digipen.edu
\course:    CSD 3451 - Software Engineering Project 6
\brief:     This file implements the class for the button component.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
**************************************************************************/
#pragma once
#include <functional>
#include <string>

struct Button
{
    Button() noexcept = default;
    Button(float x, float y, float width, float height, const std::string& text);

    void setOnClick(std::function<void()> callback);

    bool isHovered() const noexcept;

    float x{};
    float y{};
    float width{};
    float height{};
    std::string text{};

    bool hovered{};
    bool pressed{};

    std::function<void()> onClick{};
};
