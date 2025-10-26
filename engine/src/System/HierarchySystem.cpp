#include "System/HierarchySystem.hpp"
#include "Component/Transform.hpp"
#include "Component/RelationshipComponent.hpp"
#include <glm/glm.hpp>

void HierarchySystem::Init()
{
	// Nothing to initialize
}

void HierarchySystem::Update(ecs::world& world, float dt)
{
	// Update all root entities first (entities without parents or with invalid parents)
	UpdateRootEntities(world);
}

void HierarchySystem::Exit()
{
	// Nothing to clean up
}

void HierarchySystem::UpdateRootEntities(ecs::world& world)
{
	// Find all entities with both TransformComponent and TransformMtxComponent
	auto view = world.impl.get_registry().view<TransformComponent, TransformMtxComponent>();

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
			auto& mtx = ent.get<TransformMtxComponent>();

			// Root entities: world matrix = local matrix (from m_Translation, m_Rotation, m_Scale)
			if (transform.isDirty)
			{
				mtx.m_Mtx = transform.getLocalMatrix();
				transform.isDirty = false;
			}

			// Recursively update children
			if (hasRelationship)
			{
				auto& relationship = ent.get<RelationshipComponent>();
				if (relationship.getChildCount() > 0)
				{
					// Pass world, entity, and parent's world matrix to children
					UpdateTransformHierarchy(world, ent, mtx.m_Mtx);
				}
			}
		}
	}
}

void HierarchySystem::UpdateTransformHierarchy(ecs::world& world, ecs::entity entity, const glm::mat4& parentWorldMatrix)
{
	if (!entity.all<RelationshipComponent>())
		return;

	auto& relationship = entity.get<RelationshipComponent>();
	const auto& children = relationship.getChildren();

	for (auto childEntity : children)
	{
		// Validate child entity exists and has required components
		if (!world.is_valid(childEntity))
			continue;

		if (!childEntity.all<TransformComponent, TransformMtxComponent>())
			continue;

		auto& childTransform = childEntity.get<TransformComponent>();
		auto& mtx = childEntity.get<TransformMtxComponent>();

		// Update child's world matrix: parent's world matrix * child's local matrix
		// getLocalMatrix() uses m_Translation, m_Rotation (Euler), and m_Scale
		mtx.m_Mtx = parentWorldMatrix * childTransform.getLocalMatrix();
		childTransform.isDirty = false;

		// Recursively update this child's children
		UpdateTransformHierarchy(world, childEntity, mtx.m_Mtx);
	}
}

// Register the system
RegisterSystemDerived(
	HierarchySystem,
	HierarchySystem,
	(ecs::ReadSet<RelationshipComponent>),
	(ecs::WriteSet<TransformComponent, TransformMtxComponent>),
	60.0f // Update 60 times per second
);
