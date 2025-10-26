#ifndef HIERARCHY_SYSTEM_HPP
#define HIERARCHY_SYSTEM_HPP

#include "ecs/system/system.h"
#include "ecs/internal/world.h"
#include <glm/glm.hpp>

/**
 * @brief System that propagates transforms through entity hierarchies
 *
 * The HierarchySystem updates world matrices for entities in parent-child relationships.
 * It computes world space transformations by multiplying parent world matrices with
 * child local matrices.
 *
 * @requirements
 * Entities MUST have both TransformComponent AND TransformMtxComponent before being
 * processed by this system. Entities missing either component will be skipped.
 *
 * @usage
 * @code
 * // Create entity with required components
 * ecs::entity entity = world.add_entity();
 * entity.add<TransformComponent>();      // Stores local transform
 * entity.add<TransformMtxComponent>();   // Stores computed world matrix
 * entity.add<RelationshipComponent>();   // For parent-child relationships
 *
 * // Set up hierarchy using SceneGraph API
 * SceneGraph::SetParent(child, parent);
 *
 * // HierarchySystem will automatically update world matrices each frame
 * @endcode
 */
class HierarchySystem : public ecs::SystemBase
{
public:
	void Init() override;
	void Update(ecs::world& world, float dt) override;
	void Exit() override;

private:
	// Recursively update transform hierarchy starting from root entities
	void UpdateTransformHierarchy(ecs::world& world, ecs::entity entity, const glm::mat4& parentWorldMatrix);

	// Update all root entities (entities without parents)
	void UpdateRootEntities(ecs::world& world);
};

#endif // HIERARCHY_SYSTEM_HPP
