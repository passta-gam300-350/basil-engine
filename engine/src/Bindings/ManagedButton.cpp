/******************************************************************************/
/*!
\file   ManagedButton.cpp
\author Team PASSTA
\par    Course : CSD3451 / UXG3450
\date   2026/02/28
\brief Managed bindings for ButtonComponent state queries.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Bindings/ManagedButton.hpp"

#include "Input/Button.h"
#include "ecs/internal/entity.h"

bool ManagedButton::GetHovered(uint64_t handle)
{
    ecs::entity entity{ handle };
    return entity.any<Button>() ? entity.get<Button>().hovered : false;
}

bool ManagedButton::GetPressed(uint64_t handle)
{
    ecs::entity entity{ handle };
    return entity.any<Button>() ? entity.get<Button>().pressed : false;
}

bool ManagedButton::GetClicked(uint64_t handle)
{
    ecs::entity entity{ handle };
    return entity.any<Button>() ? entity.get<Button>().clicked : false;
}
