#pragma once
/*!************************************************************************
\file:      JoltDebugRenderer.h
\author:    Bryan Ang Wei Ze
\email:     bryanweize.ang@digipen.edu
\course:    CSD 3401 - Software Engineering Project 5
\brief:     Jolt Physics debug renderer adapter for integration with the
            engine's rendering pipeline. Converts Jolt debug draw calls
            into DebugLine primitives for visualization.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
***************************************************************************/

// Jolt includes
#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRenderer.h>

// Graphics library includes
#include <../lib/Graphics/include/Utility/FrameData.h>

// STL includes
#include <vector>
#include <glm/glm.hpp>

/**
 * @brief Jolt Physics debug renderer implementation for the engine
 *
 * This class implements Jolt's DebugRenderer interface to visualize physics
 * objects (collision shapes, velocities, contact points, etc.). It buffers
 * debug geometry during physics updates and flushes to FrameData for rendering.
 *
 * Usage:
 *   1. Instantiate in PhysicsSystem::Init()
 *   2. Register as Jolt singleton: JPH::DebugRenderer::sInstance = this
 *   3. Call Jolt debug draw APIs during physics update
 *   4. Call FlushToFrameData() before rendering
 *   5. DebugRenderPass will render the lines
 */
class JoltDebugRenderer : public JPH::DebugRenderer
{
public:
    /**
     * @brief Constructor
     */
    JoltDebugRenderer();

    /**
     * @brief Destructor - cleans up resources
     */
    ~JoltDebugRenderer();

    /**
     * @brief Flush buffered debug geometry to FrameData for rendering
     * @param frameData The frame data to submit debug lines to
     *
     * Call this in RenderSystem::Update() before SceneRenderer->Render().
     * Clears the internal line buffer after flushing.
     */
    void FlushToFrameData(FrameData& frameData);

    /**
     * @brief Clear all buffered debug geometry
     */
    void Clear();

    /**
     * @brief Enable or disable debug rendering
     * @param enabled True to enable, false to disable
     */
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * @brief Check if debug rendering is enabled
     * @return True if enabled, false otherwise
     */
    bool IsEnabled() const { return m_enabled; }

    // =============================================================================
    // JPH::DebugRenderer Interface Implementation
    // =============================================================================

    /**
     * @brief Draw a single line (primary method for physics debug visualization)
     * @param inFrom Start position in world space
     * @param inTo End position in world space
     * @param inColor Line color (Jolt Color, 0-255 range)
     */
    virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;

    /**
     * @brief Draw a single triangle (stub - not implemented for basic line rendering)
     * @param inV1 First vertex
     * @param inV2 Second vertex
     * @param inV3 Third vertex
     * @param inColor Triangle color
     * @param inCastShadow Whether the triangle should cast shadows
     */
    virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3,
                              JPH::ColorArg inColor, ECastShadow inCastShadow = ECastShadow::Off) override;

    /**
     * @brief Create a triangle batch from triangle array (stub - not implemented)
     * @param inTriangles Array of triangles
     * @param inTriangleCount Number of triangles
     * @return Batch handle (null in this implementation)
     */
    virtual Batch CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) override;

    /**
     * @brief Create a triangle batch from vertices and indices (stub - not implemented)
     * @param inVertices Array of vertices
     * @param inVertexCount Number of vertices
     * @param inIndices Array of indices
     * @param inIndexCount Number of indices
     * @return Batch handle (null in this implementation)
     */
    virtual Batch CreateTriangleBatch(const Vertex* inVertices, int inVertexCount,
                                       const uint32_t* inIndices, int inIndexCount) override;

    /**
     * @brief Draw batched geometry (stub - not implemented)
     * @param inModelMatrix Transform matrix
     * @param inWorldSpaceBounds Bounding box in world space
     * @param inLODScaleSq LOD scale squared
     * @param inModelColor Model color
     * @param inGeometry Geometry batch to draw
     * @param inCullMode Culling mode
     * @param inCastShadow Whether to cast shadows
     * @param inDrawMode Drawing mode (solid/wireframe)
     */
    virtual void DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds,
                               float inLODScaleSq, JPH::ColorArg inModelColor,
                               const GeometryRef& inGeometry, ECullMode inCullMode = ECullMode::CullBackFace,
                               ECastShadow inCastShadow = ECastShadow::Off,
                               EDrawMode inDrawMode = EDrawMode::Solid) override;

    /**
     * @brief Draw 3D text (stub - not implemented)
     * @param inPosition Position in world space
     * @param inString Text to draw
     * @param inColor Text color
     * @param inHeight Text height
     */
    virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString,
                             JPH::ColorArg inColor, float inHeight = 0.5f) override;

private:
    /**
     * @brief Convert Jolt RVec3 to GLM vec3
     * @param jphVec Jolt vector
     * @return GLM vector
     */
    static glm::vec3 JoltToGLM(JPH::RVec3Arg jphVec);

    /**
     * @brief Convert Jolt Color to GLM vec3 (normalized 0.0-1.0)
     * @param jphColor Jolt color (0-255 range)
     * @return GLM color vector (0.0-1.0 range)
     */
    static glm::vec3 JoltColorToGLM(JPH::ColorArg jphColor);

private:
    /**
     * @brief Triangle for wireframe rendering (renamed to avoid conflict with Jolt's Triangle)
     */
    struct StoredTriangle {
        glm::vec3 v1, v2, v3;
    };

    /**
     * @brief Batch geometry storage for collision shapes
     *
     * Stores triangle geometry created by Jolt's Initialize() method.
     * Used by DrawGeometry() to render collision shapes as wireframe lines.
     */
    struct BatchImpl : public JPH::RefTargetVirtual {
        std::vector<StoredTriangle> triangles;      ///< Direct triangle format
        std::vector<glm::vec3> vertices;      ///< Indexed format vertices
        std::vector<uint32_t> indices;        ///< Indexed format indices

        virtual void AddRef() override { ++m_RefCount; }
        virtual void Release() override { if (--m_RefCount == 0) delete this; }

    private:
        std::atomic<uint32_t> m_RefCount = 0;  // Start at 0 (Jolt's Ref<> calls AddRef())
    };

    std::vector<DebugLine> m_lineBuffer;  ///< Buffered debug lines
    bool m_enabled;                        ///< Whether debug rendering is enabled
};
