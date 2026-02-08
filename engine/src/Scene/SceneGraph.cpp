/******************************************************************************/
/*!
\file   SceneGraph.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Scene graph implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Scene/SceneGraph.hpp"
#include "Component/RelationshipComponent.hpp"
#include "Component/Transform.hpp"
#include <glm/gtx/matrix_decompose.hpp>

void SceneGraph::SetParent(ecs::entity child, ecs::entity parent, bool keepWorldTransform)
{
	if (!static_cast<bool>(child))
		return;

	// Get or add RelationshipComponent to child
	RelationshipComponent* childRelationship = nullptr;
	if (!child.all<RelationshipComponent>())
	{
		childRelationship = &child.add<RelationshipComponent>();
	}
	else
	{
		childRelationship = &child.get<RelationshipComponent>();
	}

	// Remove from old parent if exists
	if (childRelationship->hasParent())
	{
		ecs::entity oldParent = childRelationship->getParent();
		if (static_cast<bool>(oldParent) && oldParent.all<RelationshipComponent>())
		{
			auto& oldParentRel = oldParent.get<RelationshipComponent>();
			oldParentRel.removeChild(child);
		}
	}

	// Store current world transform if needed
	glm::mat4 currentWorldTransform;
	if (keepWorldTransform)
	{
		currentWorldTransform = GetWorldTransform(child);
	}

	// Set new parent
	if (static_cast<bool>(parent))
	{
		childRelationship->setParent(parent);

		// Get or add RelationshipComponent to parent
		if (!parent.all<RelationshipComponent>())
		{
			parent.add<RelationshipComponent>();
		}
		auto& parentRelationship = parent.get<RelationshipComponent>();
		parentRelationship.addChild(child);
	}
	else
	{
		childRelationship->setParent(ecs::entity{}); // Invalid entity = no parent
	}
	// Adjust local transform to maintain world position if requested
	if (keepWorldTransform)
	{
		ConvertWorldToLocal(child, currentWorldTransform);
	}

	MarkTransformDirty(child);
}

void SceneGraph::RemoveParent(ecs::entity entity, bool keepWorldTransform)
{
	SetParent(entity, ecs::entity{}, keepWorldTransform);
}

void SceneGraph::AddChild(ecs::entity parent, ecs::entity child, bool keepWorldTransform)
{
	SetParent(child, parent, keepWorldTransform);
}

void SceneGraph::RemoveChild(ecs::entity parent, ecs::entity child, bool keepWorldTransform)
{
	if (!static_cast<bool>(parent) || !static_cast<bool>(child))
		return;

	if (parent.all<RelationshipComponent>())
	{
		auto& parentRelationship = parent.get<RelationshipComponent>();
		parentRelationship.removeChild(child);
	}

	if (child.all<RelationshipComponent>())
	{
		auto& childRelationship = child.get<RelationshipComponent>();
		childRelationship.setParent(ecs::entity{});
	}

	if (keepWorldTransform && child.all<TransformComponent>() && child.all<TransformMtxComponent>())
	{
		auto& transform = child.get<TransformComponent>();
		auto& mtx = child.get<TransformMtxComponent>();

		// Convert world transform to local
		glm::vec3 scale, translation, skew;
		glm::vec4 perspective;
		glm::quat rotation;
		glm::decompose(mtx.m_Mtx, scale, rotation, translation, skew, perspective);

		transform.m_Translation = translation;
		transform.m_Rotation = glm::degrees(glm::eulerAngles(rotation));
		transform.m_Scale = scale;
	}

	MarkTransformDirty(child);
}

std::vector<ecs::entity> SceneGraph::GetChildren(ecs::entity entity)
{
	if (!static_cast<bool>(entity))
		return {};

	if (!entity.all<RelationshipComponent>())
		return {};

	auto& relationship = entity.get<RelationshipComponent>();
	return relationship.getChildren();
}

ecs::entity SceneGraph::GetParent(ecs::entity entity)
{
	if (!static_cast<bool>(entity))
		return {};

	if (!entity.all<RelationshipComponent>())
		return {};

	auto& relationship = entity.get<RelationshipComponent>();
	return relationship.getParent();
}

bool SceneGraph::HasParent(ecs::entity entity)
{
	if (!static_cast<bool>(entity))
		return false;

	if (!entity.all<RelationshipComponent>())
		return false;

	auto& relationship = entity.get<RelationshipComponent>();
	return relationship.hasParent();
}

glm::mat4 SceneGraph::GetWorldTransform(ecs::entity entity)
{
	if (!static_cast<bool>(entity))
		return glm::mat4(1.0f);

	if (!entity.all<TransformMtxComponent>())
		return glm::mat4(1.0f);

	auto& mtx = entity.get<TransformMtxComponent>();
	return mtx.m_Mtx;
}

glm::mat4 SceneGraph::GetLocalTransform(ecs::entity entity)
{
	if (!static_cast<bool>(entity))
		return glm::mat4(1.0f);

	if (!entity.all<TransformComponent>())
		return glm::mat4(1.0f);

	auto& transform = entity.get<TransformComponent>();
	return transform.getLocalMatrix();
}

void SceneGraph::MarkTransformDirty(ecs::entity entity)
{
	if (!static_cast<bool>(entity))
		return;

	if (entity.all<TransformComponent>())
	{
		auto& transform = entity.get<TransformComponent>();
		transform.isDirty = true;
	}
}

void SceneGraph::MarkHierarchyDirty(ecs::entity entity)
{
	MarkTransformDirty(entity);

	auto children = GetChildren(entity);
	for (const auto& child : children)
	{
		MarkHierarchyDirty(child);
	}
}

void SceneGraph::ConvertWorldToLocal(ecs::entity entity, const glm::mat4& targetWorldMatrix)
{
	if (!static_cast<bool>(entity))
		return;

	if (!entity.all<TransformComponent>())
		return;

	auto& transform = entity.get<TransformComponent>();

	// Get parent's transform components
	ecs::entity parent = GetParent(entity);
	if (!static_cast<bool>(parent) || !parent.all<TransformComponent>())
	{
		// No parent or parent has no transform, just extract from target
		glm::vec3 scale, translation, skew;
		glm::vec4 perspective;
		glm::quat rotation;
		glm::decompose(targetWorldMatrix, scale, rotation, translation, skew, perspective);

		transform.m_Translation = translation;
		transform.m_Rotation = glm::degrees(glm::eulerAngles(rotation));
		transform.m_Scale = scale;
		transform.isDirty = true;
		return;
	}

	// Extract parent's world transform components
	if (!parent.all<TransformMtxComponent>())
		return; // Parent has no world matrix yet

	auto& parentMtx = parent.get<TransformMtxComponent>();
	glm::vec3 parentScale, parentTranslation, parentSkew;
	glm::vec4 parentPerspective;
	glm::quat parentRotation;
	glm::decompose(parentMtx.m_Mtx, parentScale, parentRotation, parentTranslation, parentSkew, parentPerspective);

	// Extract target world transform components
	glm::vec3 targetScale, targetTranslation, targetSkew;
	glm::vec4 targetPerspective;
	glm::quat targetRotation;
	glm::decompose(targetWorldMatrix, targetScale, targetRotation, targetTranslation, targetSkew, targetPerspective);

	// Calculate local transform relative to parent
	// Position: remove parent's translation and rotation, then divide by parent's scale
	glm::vec3 relativePos = targetTranslation - parentTranslation;
	glm::mat3 inverseParentRotation = glm::mat3_cast(glm::inverse(parentRotation));
	relativePos = inverseParentRotation * relativePos;
	relativePos /= parentScale; // Divide by parent scale to get local offset

	// Rotation: relative to parent
	glm::quat relativeRotation = glm::inverse(parentRotation) * targetRotation;

	// Scale: relative to parent
	glm::vec3 relativeScale = targetScale / parentScale;

	transform.m_Translation = relativePos;
	transform.m_Rotation = glm::degrees(glm::eulerAngles(relativeRotation));
	transform.m_Scale = relativeScale;
	transform.isDirty = true;
}

void SceneGraph::ConvertLocalToWorld(ecs::entity entity)
{
	if (!static_cast<bool>(entity))
		return;

	// Require both components - do not auto-add
	if (!entity.all<TransformComponent, TransformMtxComponent>())
		return;

	auto& transform = entity.get<TransformComponent>();
	auto& mtx = entity.get<TransformMtxComponent>();

	// Get parent's world matrix
	glm::mat4 parentWorldMatrix = glm::mat4(1.0f);
	ecs::entity parent = GetParent(entity);
	if (static_cast<bool>(parent))
	{
		parentWorldMatrix = GetWorldTransform(parent);
	}

	// Calculate world matrix: parent * local
	mtx.m_Mtx = parentWorldMatrix * transform.getLocalMatrix();
}
