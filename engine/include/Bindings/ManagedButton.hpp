/******************************************************************************/
/*!
\file   ManagedButton.hpp
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
#ifndef MANAGED_BUTTON_HPP
#define MANAGED_BUTTON_HPP

#include <cstdint>

class ManagedButton
{
public:
    static bool GetHovered(uint64_t handle);
    static bool GetPressed(uint64_t handle);
    static bool GetClicked(uint64_t handle);
};

#endif
