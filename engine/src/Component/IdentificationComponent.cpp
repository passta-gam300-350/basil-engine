#include "Component/IdentificationComponent.hpp"

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
	identifier = std::move(name);
	uuid = UUID<128>::Generate();
}

void IdentifierComponent::setName(std::string&& name)
{
	identifier = std::move(name);
	uuid = UUID<128>::Generate();
}

std::string IdentifierComponent::getName()
{
	return identifier;
}

