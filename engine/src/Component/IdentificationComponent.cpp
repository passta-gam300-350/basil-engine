#include "Component/IdentificationComponent.hpp"

#include "Manager/StringManager.hpp"

IdentifierComponent::IdentifierComponent()
{
	handle = Handle{ "IdentifierComponent" };
}

bool IdentifierComponent::inEditor()
{
	return false;
}



Component::ComponentType IdentifierComponent::getType() const
{
	return ComponentType::IDENTIFIER;
}

IdentifierComponent::IdentifierComponent(std::string&& name)
{
	handle = Handle{ "IdentifierComponent" };
	identifier = StringManager::GetInstance().Intern_String(std::move(name));
	uuid = UUID<128>::Generate();
}

void IdentifierComponent::setName(std::string&& name)
{
	identifier = StringManager::GetInstance().Intern_String(std::move(name));
	uuid = UUID<128>::Generate();
}

std::string IdentifierComponent::getName()
{
	return std::string{ StringManager::GetInstance().Get_String(identifier) };
}

