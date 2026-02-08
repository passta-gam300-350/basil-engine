/******************************************************************************/
/*!
\file   Handle.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Handle implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Handle.hpp"

#include "Manager/StringManager.hpp"
#include "uuid/uuid.hpp"

ComponentHandle::ComponentHandle(std::string name) : handle_name{StringManager::GetInstance().Intern_String(std::move(name))}, id{uuid::UUID<128>::Generate()}
{
}
ComponentHandle::ComponentHandle(std::string name, uuid::UUID<128> const& uuid) : handle_name{ StringManager::GetInstance().Intern_String(std::move(name)) }, id{uuid}
{

}
