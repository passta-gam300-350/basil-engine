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
};