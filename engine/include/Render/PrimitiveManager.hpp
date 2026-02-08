/******************************************************************************/
/*!
\file   PrimitiveManager.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Primitive mesh management

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef ENGINE_PRIMITIVE_MANAGER_HPP
#define ENGINE_PRIMITIVE_MANAGER_HPP
#pragma once

#include <Resources/Mesh.h>
#include <memory>

/**
 * @brief Manages shared primitive meshes for the render system
 *
 * Responsibilities:
 * - Create and cache primitive meshes (cube, plane)
 * - Provide shared mesh access for instancing efficiency
 * - Generate debug visualization meshes (light cube, directional ray, wireframe)
 *
 * Lifetime: Owned by RenderSystem
 */
class PrimitiveManager {
public:
    PrimitiveManager() = default;
    ~PrimitiveManager() = default;

    // Delete copy/move to prevent accidental duplication
    PrimitiveManager(const PrimitiveManager&) = delete;
    PrimitiveManager& operator=(const PrimitiveManager&) = delete;
    PrimitiveManager(PrimitiveManager&&) = delete;
    PrimitiveManager& operator=(PrimitiveManager&&) = delete;

    /**
     * @brief Initialize primitive meshes
     * Creates essential shared meshes (cube, plane)
     */
    void Initialize();

    /**
     * @brief Get shared cube mesh for instancing
     * Lazy initialization - creates mesh on first call
     * @return Shared cube mesh (1.0 unit size)
     */
    std::shared_ptr<Mesh> GetSharedCubeMesh();

    /**
     * @brief Get shared plane mesh for instancing
     * Lazy initialization - creates mesh on first call
     * @return Shared plane mesh (2.0x2.0 unit size)
     */
    std::shared_ptr<Mesh> GetSharedPlaneMesh();

    /**
     * @brief Create debug visualization mesh for point/spot lights
     * @param size Size of the light cube
     * @return Light cube mesh
     */
    std::shared_ptr<Mesh> CreateDebugLightCube(float size = 5.0f);

    /**
     * @brief Clear all cached primitive meshes
     */
    void Clear();

private:
    // Shared primitive meshes for entity rendering (cached for instancing)
    std::shared_ptr<Mesh> m_SharedCubeMesh;
    std::shared_ptr<Mesh> m_SharedPlaneMesh;

    // Helper methods
    void CreateSharedPrimitives();
};

#endif // ENGINE_PRIMITIVE_MANAGER_HPP
