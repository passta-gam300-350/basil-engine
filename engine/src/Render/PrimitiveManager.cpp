#include "Render/PrimitiveManager.hpp"
#include <Resources/PrimitiveGenerator.h>
#include <spdlog/spdlog.h>
#include <cassert>

void PrimitiveManager::Initialize() {
    spdlog::info("PrimitiveManager: Initializing...");

    // Create shared primitives eagerly
    CreateSharedPrimitives();

    spdlog::info("PrimitiveManager: Initialized successfully");
}

void PrimitiveManager::CreateSharedPrimitives() {
    // Create shared cube mesh
    if (!m_SharedCubeMesh) {
        m_SharedCubeMesh = std::make_shared<Mesh>(PrimitiveGenerator::CreateCube(1.0f));
        assert(m_SharedCubeMesh && "Cube mesh generation failed");
        assert(!m_SharedCubeMesh->vertices.empty() && "Generated cube has no vertices");
        spdlog::debug("PrimitiveManager: Created shared cube mesh");
    }

    // Create shared plane mesh
    if (!m_SharedPlaneMesh) {
        m_SharedPlaneMesh = std::make_shared<Mesh>(PrimitiveGenerator::CreatePlane(2.0f, 2.0f, 1, 1));
        assert(m_SharedPlaneMesh && "Plane mesh generation failed");
        assert(!m_SharedPlaneMesh->vertices.empty() && "Generated plane has no vertices");
        spdlog::debug("PrimitiveManager: Created shared plane mesh");
    }
}

std::shared_ptr<Mesh> PrimitiveManager::GetSharedCubeMesh() {
    // Lazy initialization fallback (if Initialize() wasn't called)
    if (!m_SharedCubeMesh) {
        spdlog::warn("PrimitiveManager: Lazy initializing cube mesh (Initialize() should be called first)");
        m_SharedCubeMesh = std::make_shared<Mesh>(PrimitiveGenerator::CreateCube(1.0f));
        assert(m_SharedCubeMesh && "Cube mesh generation failed");
        assert(!m_SharedCubeMesh->vertices.empty() && "Generated cube has no vertices");
    }
    return m_SharedCubeMesh;
}

std::shared_ptr<Mesh> PrimitiveManager::GetSharedPlaneMesh() {
    // Lazy initialization fallback (if Initialize() wasn't called)
    if (!m_SharedPlaneMesh) {
        spdlog::warn("PrimitiveManager: Lazy initializing plane mesh (Initialize() should be called first)");
        m_SharedPlaneMesh = std::make_shared<Mesh>(PrimitiveGenerator::CreatePlane(2.0f, 2.0f, 1, 1));
        assert(m_SharedPlaneMesh && "Plane mesh generation failed");
        assert(!m_SharedPlaneMesh->vertices.empty() && "Generated plane has no vertices");
    }
    return m_SharedPlaneMesh;
}

std::shared_ptr<Mesh> PrimitiveManager::CreateDebugLightCube(float size) {
    auto mesh = std::make_shared<Mesh>(PrimitiveGenerator::CreateCube(size));
    assert(mesh && "Light cube mesh generation failed");
    assert(!mesh->vertices.empty() && "Generated light cube has no vertices");
    spdlog::debug("PrimitiveManager: Created debug light cube (size: {})", size);
    return mesh;
}

std::shared_ptr<Mesh> PrimitiveManager::CreateDebugDirectionalRay(float length) {
    auto mesh = std::make_shared<Mesh>(PrimitiveGenerator::CreateDirectionalRay(length));
    assert(mesh && "Directional ray mesh generation failed");
    assert(!mesh->vertices.empty() && "Generated directional ray has no vertices");
    spdlog::debug("PrimitiveManager: Created debug directional ray (length: {})", length);
    return mesh;
}

std::shared_ptr<Mesh> PrimitiveManager::CreateDebugWireframeCube(float size) {
    auto mesh = std::make_shared<Mesh>(PrimitiveGenerator::CreateWireframeCube(size));
    assert(mesh && "Wireframe cube mesh generation failed");
    assert(!mesh->vertices.empty() && "Generated wireframe cube has no vertices");
    spdlog::debug("PrimitiveManager: Created debug wireframe cube (size: {})", size);
    return mesh;
}

void PrimitiveManager::Clear() {
    m_SharedCubeMesh.reset();
    m_SharedPlaneMesh.reset();
    spdlog::info("PrimitiveManager: Cleared all primitive meshes");
}
