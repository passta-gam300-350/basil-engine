#include "Component/RelationshipComponent.hpp"
#include <algorithm>
#include "Engine.hpp"

ecs::entity RelationshipComponent::getParent(rp::Guid guid) const
{
	rp::BasicIndexedGuid fullguid{guid, rp::utility::compute_string_hash("scene")};
	auto res{ Engine::GetSceneRegistry().GetReferencedEntity(SceneEntityReference{ fullguid, parentHandle }) };
	assert(res && "entity not found in scene!");
	return res.value();
}

Component::ComponentType RelationshipComponent::getType() const
{
	return Component::ComponentType::RELATIONSHIP;
}

void RelationshipComponent::setParent(ecs::entity newParent)
{
	if (newParent && newParent.get_uid()) {
		parentHandle = newParent.get<SceneIDComponent>().m_scene_id;
	}
}

bool RelationshipComponent::hasParent() const
{
	return static_cast<bool>(parentHandle);
}

std::vector<ecs::entity> RelationshipComponent::getChildren(rp::Guid guid) const
{
	std::vector<ecs::entity> children{};
	rp::BasicIndexedGuid fullguid{ guid, rp::utility::compute_string_hash("scene") };
	for (auto const child_sid : childrenHandles) {
		auto res{ Engine::GetSceneRegistry().GetReferencedEntity(SceneEntityReference{ fullguid, child_sid }) };
		if (!res) {
			continue;
		}
		//assert(res && "entity not found in scene!");
		children.emplace_back(res.value());
	}
	return children;
}

void RelationshipComponent::addChild(ecs::entity child)
{
	SceneEntityID child_sid = child.get<SceneIDComponent>().m_scene_id;
	// Check if child already exists
	auto it = std::find(childrenHandles.begin(), childrenHandles.end(), child_sid);
	if (it == childrenHandles.end())
	{
		childrenHandles.push_back(child_sid);
	}
}

void RelationshipComponent::removeChild(ecs::entity child)
{
	SceneEntityID child_sid = child.get<SceneIDComponent>().m_scene_id;
	auto it = std::find(childrenHandles.begin(), childrenHandles.end(), child_sid);
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


