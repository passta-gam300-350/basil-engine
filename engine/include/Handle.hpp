/******************************************************************************/
/*!
\file   Handle.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Handle types for entity and component references

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef HANDLE_HPP
#define HANDLE_HPP
#include <string>

#include "uuid/uuid.hpp"

// Renamed from Handle to ComponentHandle to avoid conflict with Windows HANDLE and the ResourceSystem Handle struct
struct ComponentHandle
{
	uuid::UUID<128> id;
	uint32_t handle_name;
	ComponentHandle() = default;
	ComponentHandle(std::string name);
	ComponentHandle(std::string name, uuid::UUID<128> const& uuid);
};

#endif // !HANDLE_HPP