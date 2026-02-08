/******************************************************************************/
/*!
\file   Identification.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Identification component implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Component/Identification.hpp"

#include "Manager/StringManager.hpp"

Identifier::Identifier()
{
	handle = ComponentHandle{ "Identifier" };
}

bool Identifier::inEditor()
{
	return false;
}



Component::ComponentType Identifier::getType() const
{
	return ComponentType::IDENTIFIER;
}

Identifier::Identifier(std::string&& name)
{
	handle = ComponentHandle{ "Identifier" };
	identifier = StringManager::GetInstance().Intern_String(std::move(name));
	uuid = uuid::UUID<128>::Generate();
}

void Identifier::setName(std::string&& name)
{
	identifier = StringManager::GetInstance().Intern_String(std::move(name));
	uuid = uuid::UUID<128>::Generate();
}

std::string Identifier::getName()
{
	return std::string{ StringManager::GetInstance().Get_String(identifier) };
}

