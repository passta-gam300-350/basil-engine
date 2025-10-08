#ifndef TRANSFORM_HIERARCHY_SYSTEM_HPP
#define TRANSFORM_HIERARCHY_SYSTEM_HPP

#include "ecs/system/system.h"
#include "ecs/internal/world.h"
#include <glm/glm.hpp>

class TransformHierarchySystem : public ecs::SystemBase
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

#endif // TRANSFORM_HIERARCHY_SYSTEM_HPP
