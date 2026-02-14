/*!************************************************************************
\file:      JoltDebugRenderer.cpp
\author:    Bryan Ang Wei Ze
\email:     bryanweize.ang@digipen.edu
\course:    CSD 3401 - Software Engineering Project 5
\brief:     Implementation of the Jolt Physics debug renderer adapter

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
***************************************************************************/

#include "Render/JoltDebugRenderer.h"

// =============================================================================
// Constructor & Destructor
// =============================================================================

JoltDebugRenderer::JoltDebugRenderer()
    : m_enabled(true)
{
    // Reserve space for typical frame (reduces allocations)
    m_lineBuffer.reserve(1024);

    // Initialize Jolt's pre-built geometry (mBox, mSphere, mCapsule, etc.)
    // This creates the geometry primitives used by DrawBox(), DrawSphere(), etc.
    Initialize();
}

JoltDebugRenderer::~JoltDebugRenderer()
{
    // Nothing to clean up (no OpenGL resources in this implementation)
}

// =============================================================================
// Public API
// =============================================================================

void JoltDebugRenderer::FlushToFrameData(FrameData& frameData)
{
    if (!m_enabled || m_lineBuffer.empty())
    {
        return;
    }

    // Transfer all buffered lines to FrameData
    frameData.debugLines.insert(frameData.debugLines.end(),
                                  m_lineBuffer.begin(),
                                  m_lineBuffer.end());

    // Clear the buffer for next frame
    m_lineBuffer.clear();
}

void JoltDebugRenderer::Clear()
{
    m_lineBuffer.clear();
}

// =============================================================================
// JPH::DebugRenderer Interface Implementation
// =============================================================================

void JoltDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
    if (!m_enabled)
    {
        return;
    }

    // Convert Jolt types to GLM and buffer the line
    glm::vec3 from = JoltToGLM(inFrom);
    glm::vec3 to = JoltToGLM(inTo);
    glm::vec3 color = JoltColorToGLM(inColor);

    m_lineBuffer.emplace_back(from, to, color);
}

void JoltDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3,
                                      JPH::ColorArg inColor, ECastShadow /*inCastShadow*/)
{
    // Stub: Draw triangle edges as lines for basic visualization
    if (!m_enabled)
    {
        return;
    }

    DrawLine(inV1, inV2, inColor);
    DrawLine(inV2, inV3, inColor);
    DrawLine(inV3, inV1, inColor);
}

JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(const JPH::DebugRenderer::Triangle* inTriangles, int inTriangleCount)
{
    BatchImpl* batch = new BatchImpl();

    // Store triangles for later rendering as wireframe lines
    for (int i = 0; i < inTriangleCount; ++i) {
        const JPH::DebugRenderer::Triangle& tri = inTriangles[i];

        // Convert triangle vertices from Jolt format to GLM
        glm::vec3 v1(tri.mV[0].mPosition.x, tri.mV[0].mPosition.y, tri.mV[0].mPosition.z);
        glm::vec3 v2(tri.mV[1].mPosition.x, tri.mV[1].mPosition.y, tri.mV[1].mPosition.z);
        glm::vec3 v3(tri.mV[2].mPosition.x, tri.mV[2].mPosition.y, tri.mV[2].mPosition.z);

        // Store triangle in batch (using our StoredTriangle struct)
        batch->triangles.push_back(StoredTriangle{v1, v2, v3});
    }

    return batch;
}

JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(const Vertex* inVertices, int inVertexCount,
                                                                   const uint32_t* inIndices, int inIndexCount)
{
    BatchImpl* batch = new BatchImpl();

    // Convert vertices from Jolt format to GLM
    batch->vertices.reserve(inVertexCount);
    for (int i = 0; i < inVertexCount; ++i) {
        const Vertex& v = inVertices[i];
        batch->vertices.push_back(glm::vec3(v.mPosition.x, v.mPosition.y, v.mPosition.z));
    }

    // Store indices for indexed rendering
    batch->indices.assign(inIndices, inIndices + inIndexCount);

    return batch;
}

void JoltDebugRenderer::DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox& /*inWorldSpaceBounds*/,
                                      float /*inLODScaleSq*/, JPH::ColorArg inModelColor,
                                      const GeometryRef& inGeometry, ECullMode /*inCullMode*/,
                                      ECastShadow /*inCastShadow*/, EDrawMode /*inDrawMode*/)
{
    if (!m_enabled || inGeometry == nullptr)
    {
        return;
    }

    // Get the Geometry object from the GeometryRef
    const JPH::DebugRenderer::Geometry* geometry = inGeometry.GetPtr();
    if (!geometry || geometry->mLODs.empty())
    {
        return;
    }

    // Select the first LOD (highest detail level)
    const JPH::DebugRenderer::LOD& lod = geometry->mLODs[0];

    // Now extract the Batch and cast to our BatchImpl type
    const BatchImpl* batch = reinterpret_cast<const BatchImpl*>(lod.mTriangleBatch.GetPtr());
    if (!batch)
    {
        return;
    }

    // Convert Jolt matrix to GLM for transformation
    glm::mat4 modelMatrix(
        inModelMatrix(0, 0), inModelMatrix(1, 0), inModelMatrix(2, 0), inModelMatrix(3, 0),
        inModelMatrix(0, 1), inModelMatrix(1, 1), inModelMatrix(2, 1), inModelMatrix(3, 1),
        inModelMatrix(0, 2), inModelMatrix(1, 2), inModelMatrix(2, 2), inModelMatrix(3, 2),
        inModelMatrix(0, 3), inModelMatrix(1, 3), inModelMatrix(2, 3), inModelMatrix(3, 3)
    );

    glm::vec3 color = JoltColorToGLM(inModelColor);

    // Render direct triangle format
    if (!batch->triangles.empty())
    {
        for (const auto& tri : batch->triangles)
        {
            // Transform vertices by model matrix
            glm::vec3 v1 = glm::vec3(modelMatrix * glm::vec4(tri.v1, 1.0f));
            glm::vec3 v2 = glm::vec3(modelMatrix * glm::vec4(tri.v2, 1.0f));
            glm::vec3 v3 = glm::vec3(modelMatrix * glm::vec4(tri.v3, 1.0f));

            // Draw triangle edges as wireframe lines
            m_lineBuffer.emplace_back(v1, v2, color);
            m_lineBuffer.emplace_back(v2, v3, color);
            m_lineBuffer.emplace_back(v3, v1, color);
        }
    }
    // Render indexed vertex format
    else if (!batch->vertices.empty() && !batch->indices.empty())
    {
        // Transform all vertices once
        std::vector<glm::vec3> transformedVertices;
        transformedVertices.reserve(batch->vertices.size());
        for (const auto& vertex : batch->vertices)
        {
            transformedVertices.push_back(glm::vec3(modelMatrix * glm::vec4(vertex, 1.0f)));
        }

        // Draw triangles using indices (3 indices per triangle)
        for (size_t i = 0; i + 2 < batch->indices.size(); i += 3)
        {
            const glm::vec3& v1 = transformedVertices[batch->indices[i]];
            const glm::vec3& v2 = transformedVertices[batch->indices[i + 1]];
            const glm::vec3& v3 = transformedVertices[batch->indices[i + 2]];

            // Draw triangle edges as wireframe lines
            m_lineBuffer.emplace_back(v1, v2, color);
            m_lineBuffer.emplace_back(v2, v3, color);
            m_lineBuffer.emplace_back(v3, v1, color);
        }
    }
}

void JoltDebugRenderer::DrawText3D(JPH::RVec3Arg /*inPosition*/, const std::string_view& /*inString*/,
                                    JPH::ColorArg /*inColor*/, float /*inHeight*/)
{
    // Stub: Not implemented for basic line rendering
    // 3D text rendering would require a font system
}

// =============================================================================
// Private Helper Methods
// =============================================================================

glm::vec3 JoltDebugRenderer::JoltToGLM(JPH::RVec3Arg jphVec)
{
    return glm::vec3(
        static_cast<float>(jphVec.GetX()),
        static_cast<float>(jphVec.GetY()),
        static_cast<float>(jphVec.GetZ())
    );
}

glm::vec3 JoltDebugRenderer::JoltColorToGLM(JPH::ColorArg jphColor)
{
    // Convert from 0-255 range to 0.0-1.0 range
    return glm::vec3(
        jphColor.r / 255.0f,
        jphColor.g / 255.0f,
        jphColor.b / 255.0f
    );
}
