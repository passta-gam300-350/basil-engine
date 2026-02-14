/******************************************************************************/
/*!
\file   RelationshipComponent.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Relationship component for entity hierarchy

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef RELATONSHIPCOMPONENT_HPP
#define RELATONSHIPCOMPONENT_HPP

#define NO_DEF_UUID
#include "Component.hpp"
#include "ecs/ecs.h"
#include <vector>
#include "Scene/Scene.hpp"

class RelationshipComponent : public Component
{
public:
	SceneEntityID parentHandle;
	std::vector<SceneEntityID> childrenHandles;

	RelationshipComponent() = default;
	RelationshipComponent(ecs::entity parent) : parentHandle{ parent } {}
	~RelationshipComponent() override = default;
	ComponentType getType() const override;

	// Parent management
	ecs::entity getParent(rp::Guid = rp::null_guid) const;
	void setParent(ecs::entity newParent);
	bool hasParent() const;

	// Children management
	std::vector<ecs::entity> getChildren(rp::Guid = rp::null_guid) const;
	void addChild(ecs::entity child);
	void removeChild(ecs::entity child);
	void clearChildren();
	size_t getChildCount() const;

	bool inEditor() override;

};

RegisterReflectionTypeBegin(RelationshipComponent, "Relationship")
	MemberRegistrationV<&RelationshipComponent::parentHandle, "parent">,
	MemberRegistrationV<&RelationshipComponent::childrenHandles, "child">
RegisterReflectionTypeEnd
//
//RegisterReflectionTypeBegin(RelationshipComponent, "Relationship")
//	MemberRegistrationV<&RelationshipComponent::parentHandle, "parent">,
//	MemberRegistrationV<&RelationshipComponent::childrenHandles, "parent">
//RegisterReflectionTypeEnd


#endif //RELATONSHIPCOMPONENT_HPP
