#pragma once
#ifndef __BVH_RENDERABLE_H__
#define __BVH_RENDERABLE_H__

/******************************************************************************/
/*!
\file   bvh_renderable.h
\author Team PASSTA
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : GAM300
\date   2025/11/01
\brief  Wrapper struct that bridges ECS entities with BVH spatial index

This file defines the BvhRenderable struct which acts as an adapter between
the engine's ECS entities and the BVH template requirements.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "BVH/shapes.h"
#include "BVH/bvh.h"
#include "Ecs/ecs.h"

// Forward declaration to avoid circular dependency
template<typename T> class Bvh;

/**
 * @brief Wrapper for ECS entities in BVH spatial index
 *
 * This struct adapts ECS entities to meet BVH template requirements.
 * Each BvhRenderable represents one renderable entity in the scene.
 *
 * The BVH template requires objects to have:
 * - Aabb bv (bounding volume)
 * - unsigned id (unique identifier)
 * - bvhInfo struct (internal linked list management)
 *
 * Usage:
 * 1. RenderSystem creates one BvhRenderable per renderable entity
 * 2. BVH stores pointers to these wrappers (not the entities directly)
 * 3. Query returns entity IDs, which are used to look up the wrapper
 * 4. Wrapper provides reference back to original entity
 *
 * @note RenderSystem owns the memory for these wrappers (via unique_ptr)
 */
struct BvhRenderable 
{
    // ========== Required by BVH Template ==========

    /**
     * @brief World-space axis-aligned bounding box
     *
     * Computed from mesh local AABB + entity transform.
     * Updated when entity moves (for dynamic objects).
     */
    Aabb bv;

    /**
     * @brief Unique identifier (entity UID)
     *
     * BVH queries return this ID. Used to:
     * - Look up the wrapper in RenderSystem::m_BvhObjects map
     * - Get back to the original ECS entity
     */
    unsigned id;

    /**
     * @brief BVH internal metadata (DO NOT TOUCH)
     *
     * The BVH uses this to maintain linked lists of objects in leaf nodes.
     * - next/prev: Links to other objects in same leaf
     * - node: Which leaf node contains this object
     *
     * You should NEVER manually modify these fields.
     * The BVH manages them automatically during Insert/Remove operations.
     */
    struct 
    {
        BvhRenderable* next;  ///< Next object in leaf's linked list
        BvhRenderable* prev;  ///< Previous object in leaf's linked list
        Bvh<BvhRenderable*>::Node* node;  ///< Leaf node containing this object
    } bvhInfo;

};
#endif // !__BVH_RENDERABLE_H__
