#include "Component/RelationshipComponent.hpp"
#include <algorithm>

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

bool RelationshipComponent::hasParent() const
{
	return static_cast<bool>(parentHandle);
}

const std::vector<ecs::entity>& RelationshipComponent::getChildren() const
{
	return childrenHandles;
}

void RelationshipComponent::addChild(ecs::entity child)
{
	// Check if child already exists
	auto it = std::find(childrenHandles.begin(), childrenHandles.end(), child);
	if (it == childrenHandles.end())
	{
		childrenHandles.push_back(child);
	}
}

void RelationshipComponent::removeChild(ecs::entity child)
{
	auto it = std::find(childrenHandles.begin(), childrenHandles.end(), child);
	if (it != childrenHandles.end())
	{
		childrenHandles.erase(it);
	}
}

void RelationshipComponent::clearChildren()
{
	childrenHandles.clear();
}

size_t RelationshipComponent::getChildCount() const
{
	return childrenHandles.size();
}

bool RelationshipComponent::inEditor()
{
	return false;
}


