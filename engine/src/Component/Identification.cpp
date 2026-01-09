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

