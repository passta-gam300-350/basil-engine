#include "Component/RelationshipComponent.hpp"

ecs::entity RelationshipComponent::getParent() const
{
	return parentHandle;
}

Component::ComponentType RelationshipComponent::getType() const
{
	return Component::ComponentType::RELATIONSHIP;
}

void RelationshipComponent::setParent(ecs::entity newParent)
{
	parentHandle = newParent;
}

bool RelationshipComponent::inEditor()
{
	return false;
}


