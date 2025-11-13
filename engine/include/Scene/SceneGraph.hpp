/******************************************************************************/
/*!
\file   scenegraph.h
\author Team PASSTA
		Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/09
\brief    Declares the scenegraph class

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef SCENE_GRAPH_HPP
#define SCENE_GRAPH_HPP

#include "ecs/ecs.h"
#include "ecs/internal/world.h"
#include <glm/glm.hpp>

/**
 * @brief Helper class for managing scene graph hierarchy (parent-child relationships)
 * This provides Unity-style scene hierarchy management with transform propagation
 */
class SceneGraph
{
public:
	/**
	 * @brief Set the parent of an entity
	 * @param child The child entity
	 * @param parent The new parent entity (use invalid entity to remove parent)
	 * @param keepWorldTransform If true, adjusts local transform to maintain world position
	 */
	static void SetParent(ecs::entity child, ecs::entity parent, bool keepWorldTransform = false);

	/**
	 * @brief Remove the parent from an entity (make it a root entity)
	 * @param entity The entity to make root
	 * @param keepWorldTransform If true, converts world transform to local transform
	 */
	static void RemoveParent(ecs::entity entity, bool keepWorldTransform = false);

	/**
	 * @brief Add a child to a parent entity
	 * @param parent The parent entity
	 * @param child The child entity to add
	 * @param keepWorldTransform If true, adjusts child's local transform to maintain world position
	 */
	static void AddChild(ecs::entity parent, ecs::entity child, bool keepWorldTransform = false);

	/**
	 * @brief Remove a child from a parent entity
	 * @param parent The parent entity
	 * @param child The child entity to remove
	 * @param keepWorldTransform If true, converts child's world transform to local
	 */
	static void RemoveChild(ecs::entity parent, ecs::entity child, bool keepWorldTransform = false);

	/**
	 * @brief Get all children of an entity
	 * @param entity The parent entity
	 * @return Vector of child entities (empty if no RelationshipComponent)
	 */
	static std::vector<ecs::entity> GetChildren(ecs::entity entity);

	/**
	 * @brief Get the parent of an entity
	 * @param entity The child entity
	 * @return The parent entity (invalid if no parent)
	 */
	static ecs::entity GetParent(ecs::entity entity);

	/**
	 * @brief Check if an entity has a parent
	 * @param entity The entity to check
	 * @return True if entity has a parent
	 */
	static bool HasParent(ecs::entity entity);

	/**
	 * @brief Get the world transform matrix of an entity
	 * @param entity The entity
	 * @return The world transform matrix
	 */
	static glm::mat4 GetWorldTransform(ecs::entity entity);

	/**
	 * @brief Get the local transform matrix of an entity
	 * @param entity The entity
	 * @return The local transform matrix
	 */
	static glm::mat4 GetLocalTransform(ecs::entity entity);

	/**
	 * @brief Mark an entity's transform as dirty (needs recalculation)
	 * This will automatically propagate to all children
	 * @param entity The entity to mark dirty
	 */
	static void MarkTransformDirty(ecs::entity entity);

	/**
	 * @brief Recursively mark entity and all descendants as dirty
	 * @param entity The root entity
	 */
	static void MarkHierarchyDirty(ecs::entity entity);

private:
	// Helper to convert between local and world transforms
	static void ConvertWorldToLocal(ecs::entity entity, const glm::mat4& targetWorldMatrix);
	static void ConvertLocalToWorld(ecs::entity entity);
};

#endif // SCENE_GRAPH_HPP
