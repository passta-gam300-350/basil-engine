/******************************************************************************/
/*!
\file   PrimitiveGenerator.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the PrimitiveGenerator class for generating primitive geometry meshes

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "Mesh.h"
#include <memory>

/**
 * @brief Utility class for generating primitive geometry meshes
 *
 * This class provides static methods to create common 3D primitives
 * that are compatible with the existing Mesh and rendering system.
 * All primitives follow OpenGL conventions:
 * - Counter-clockwise winding order for front faces
 * - Right-handed coordinate system
 * - Y-axis pointing up
 */
class PrimitiveGenerator
{
public:
    /**
     * @brief Creates a plane mesh in the XZ plane (Y-up)
     *
     * The plane is centered at the origin with normal pointing up (positive Y).
     * Texture coordinates follow OpenGL convention: (0,0) at bottom-left, (1,1) at top-right.
     * Triangles are wound counter-clockwise when viewed from above.
     *
     * @param width Width of the plane (X-axis extent)
     * @param height Height/depth of the plane (Z-axis extent)
     * @param subdivisionsX Number of subdivisions along X-axis (minimum 1)
     * @param subdivisionsZ Number of subdivisions along Z-axis (minimum 1)
     * @return Mesh object containing the plane geometry
     */
    static Mesh CreatePlane(
        float width = 10.0f,
        float height = 10.0f,
        int subdivisionsX = 1,
        int subdivisionsZ = 1
    );

    /**
     * @brief Creates a cube mesh centered at the origin
     *
     * The cube is centered at the origin with faces aligned to the coordinate axes.
     * All faces have outward-facing normals with counter-clockwise winding.
     * Each face has proper texture coordinates and tangent vectors.
     *
     * @param size Side length of the cube (default 1.0f)
     * @return Mesh object containing the cube geometry
     */
    static Mesh CreateCube(float size = 1.0f);

    /**
     * @brief Creates a wireframe cube mesh centered at the origin
     *
     * Creates a cube wireframe with 12 edges as line segments.
     * Perfect for debug visualization of bounding boxes and volumes.
     * Uses GL_LINES primitive type for rendering.
     *
     * @param size Side length of the cube (default 1.0f)
     * @return Mesh object containing the wireframe cube geometry
     */
    static Mesh CreateWireframeCube(float size = 1.0f);

    /**
     * @brief Creates a fullscreen quad for screen-space rendering
     *
     * Creates a quad covering the entire screen in NDC space (-1 to 1).
     * Perfect for post-processing, compositing, and screen-space effects.
     * Texture coordinates map to (0,0) bottom-left to (1,1) top-right.
     *
     * @return Mesh object containing the fullscreen quad geometry
     */
    static Mesh CreateFullscreenQuad();

    /**
     * @brief Creates a single directional ray for light direction visualization
     *
     * Creates a simple line pointing along the negative Z-axis (forward direction).
     * Used for both spot and directional light direction visualization.
     *
     * @param length Length of the ray (light range visualization)
     * @return Mesh object containing single line geometry for direction visualization
     */
    static Mesh CreateDirectionalRay(float length = 5.0f);

};