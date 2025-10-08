#include "System/TransformHierarchySystem.hpp"
#include "components/transform.h"
#include "Component/RelationshipComponent.hpp"
#include <glm/glm.hpp>

void TransformHierarchySystem::Init()
{
	// Nothing to initialize
}

void TransformHierarchySystem::Update(ecs::world& world, float dt)
{
	// Update all root entities first (entities without parents or with invalid parents)
	UpdateRootEntities(world);
}

void TransformHierarchySystem::Exit()
{
	// Nothing to clean up
}

void TransformHierarchySystem::UpdateRootEntities(ecs::world& world)
{
	// Find all entities with TransformComponent
	auto view = world.impl.get_registry().view<TransformComponent>();

	for (auto enttEntity : view)
	{
		ecs::entity ent = world.impl.entity_cast(enttEntity);

		// If no relationship or no parent, it's a root entity
		bool hasRelationship = ent.all<RelationshipComponent>();
		bool isRoot = !hasRelationship;

		if (hasRelationship)
		{
			auto& relationship = ent.get<RelationshipComponent>();
			isRoot = !relationship.hasParent();
		}

		if (isRoot)
		{
			auto& transform = ent.get<TransformComponent>();

			// Root entities: world matrix = local matrix
			if (transform.isDirty)
			{
				transform.worldMatrix = transform.getLocalMatrix();
				transform.m_trans = transform.worldMatrix; // Update legacy field
				transform.isDirty = false;
			}

			// Recursively update children
			if (hasRelationship)
			{
				auto& relationship = ent.get<RelationshipComponent>();
				if (relationship.getChildCount() > 0)
				{
					UpdateTransformHierarchy(world, ent, transform.worldMatrix);
				}
			}
		}
	}
}

void TransformHierarchySystem::UpdateTransformHierarchy(ecs::world& world, ecs::entity entity, const glm::mat4& parentWorldMatrix)
{
	if (!entity.all<RelationshipComponent>())
		return;

	auto& relationship = entity.get<RelationshipComponent>();
	const auto& children = relationship.getChildren();

	for (const auto& childEntity : children)
	{
		// Validate child entity exists
		if (!world.is_valid(childEntity))
			continue;

		if (!childEntity.all<TransformComponent>())
			continue;

		auto& childTransform = childEntity.get<TransformComponent>();

		// Update child's world matrix: parent's world matrix * child's local matrix
		childTransform.worldMatrix = parentWorldMatrix * childTransform.getLocalMatrix();
		childTransform.m_trans = childTransform.worldMatrix; // Update legacy field
		childTransform.isDirty = false;

		// Recursively update this child's children
		UpdateTransformHierarchy(world, childEntity, childTransform.worldMatrix);
	}
}

// Register the system
RegisterSystemDerived(
	TransformHierarchySystem,
	TransformHierarchySystem,
	(ecs::ReadSet<RelationshipComponent>),
	(ecs::WriteSet<TransformComponent>),
	60.0f // Update 60 times per second
);
