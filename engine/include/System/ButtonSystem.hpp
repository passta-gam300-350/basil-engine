/******************************************************************************/
/*!
\file   ButtonSystem.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/02/28
\brief  ECS system for updating button click detection.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "ecs/system/system.h"

class ButtonSystem : public ecs::SystemBase
{
public:
    static ButtonSystem& Instance();

    void Init() override;
    void Update(ecs::world& world, float dt) override;
    void FixedUpdate(ecs::world& world) override;
    void Exit() override;
};
