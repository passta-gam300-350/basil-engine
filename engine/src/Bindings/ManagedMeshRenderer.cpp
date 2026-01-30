/******************************************************************************/
/*!
\file   ManagedMeshRenderer.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/30
\brief  This file contains the implementation for the ManagedMeshRenderer class, which
        is responsible for managing mesh renderer and material-related functionalities
        in the managed environment.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#include "Bindings/ManagedMeshRenderer.hpp"


#include "ecs/internal/entity.h"
#include "Render/Render.h"
#include "Component/MaterialOverridesComponent.hpp"
#include <mono/metadata/object.h>

// ========== Basic Material Properties (MeshRendererComponent.material) ==========

void ManagedMeshRenderer::GetAlbedoColor(uint64_t handle, float* r, float* g, float* b)
{
    ecs::entity target{ handle };
    if (!target.any<MeshRendererComponent>()) {
        if (r) *r = 1.0f;
        if (g) *g = 1.0f;
        if (b) *b = 1.0f;
        return;
    }

    auto& meshRenderer = target.get<MeshRendererComponent>();
    if (r) *r = meshRenderer.material.m_AlbedoColor.r;
    if (g) *g = meshRenderer.material.m_AlbedoColor.g;
    if (b) *b = meshRenderer.material.m_AlbedoColor.b;
}

void ManagedMeshRenderer::SetAlbedoColor(uint64_t handle, float r, float g, float b)
{
    ecs::entity target{ handle };
    if (!target.any<MeshRendererComponent>()) {
        return;
    }

    auto& meshRenderer = target.get<MeshRendererComponent>();
    meshRenderer.material.m_AlbedoColor = glm::vec3(r, g, b);
}

float ManagedMeshRenderer::GetMetallic(uint64_t handle)
{
    ecs::entity target{ handle };
    if (!target.any<MeshRendererComponent>()) {
        return 0.0f;
    }

    auto& meshRenderer = target.get<MeshRendererComponent>();
    return meshRenderer.material.metallic;
}

void ManagedMeshRenderer::SetMetallic(uint64_t handle, float value)
{
    ecs::entity target{ handle };
    if (!target.any<MeshRendererComponent>()) {
        return;
    }

    auto& meshRenderer = target.get<MeshRendererComponent>();
    meshRenderer.material.metallic = value;
}

float ManagedMeshRenderer::GetRoughness(uint64_t handle)
{
    ecs::entity target{ handle };
    if (!target.any<MeshRendererComponent>()) {
        return 0.0f;
    }

    auto& meshRenderer = target.get<MeshRendererComponent>();
    return meshRenderer.material.roughness;
}

void ManagedMeshRenderer::SetRoughness(uint64_t handle, float value)
{
    ecs::entity target{ handle };
    if (!target.any<MeshRendererComponent>()) {
        return;
    }

    auto& meshRenderer = target.get<MeshRendererComponent>();
    meshRenderer.material.roughness = value;
}

// ========== Material Property Overrides (MaterialOverridesComponent) ==========

void ManagedMeshRenderer::SetFloatOverride(uint64_t handle, MonoString* propertyName, float value)
{
    ecs::entity target{ handle };

    const char* nativeName = mono_string_to_utf8(propertyName);
    std::string strName = nativeName;

    // Get or create MaterialOverridesComponent
    if (!target.any<MaterialOverridesComponent>()) {
        target.add<MaterialOverridesComponent>();
    }

    auto& overrides = target.get<MaterialOverridesComponent>();
    overrides.floatOverrides[strName] = value;
}

void ManagedMeshRenderer::SetVec3Override(uint64_t handle, MonoString* propertyName, float x, float y, float z)
{

    ecs::entity target{ handle };

    const char* nativeName = mono_string_to_utf8(propertyName);
    std::string strName = nativeName;

    // Get or create MaterialOverridesComponent
    if (!target.any<MaterialOverridesComponent>()) {
        target.add<MaterialOverridesComponent>();
    }

    auto& overrides = target.get<MaterialOverridesComponent>();
    overrides.vec3Overrides[strName] = glm::vec3(x, y, z);
}

void ManagedMeshRenderer::SetVec4Override(uint64_t handle, MonoString* propertyName, float x, float y, float z, float w)
{
    ecs::entity target{ handle };

    const char* nativeName = mono_string_to_utf8(propertyName);
    std::string strName = nativeName;


    // Get or create MaterialOverridesComponent
    if (!target.any<MaterialOverridesComponent>()) {
        target.add<MaterialOverridesComponent>();
    }

    auto& overrides = target.get<MaterialOverridesComponent>();
    overrides.vec4Overrides[strName] = glm::vec4(x, y, z, w);
}

bool ManagedMeshRenderer::HasOverride(uint64_t handle, MonoString* propertyName)
{
    ecs::entity target{ handle };

    const char* nativeName = mono_string_to_utf8(propertyName);
    std::string strName = nativeName;



    if (!target.any<MaterialOverridesComponent>()) {
        return false;
    }

    auto& overrides = target.get<MaterialOverridesComponent>();
    std::string propName(strName);

    return overrides.floatOverrides.find(propName) != overrides.floatOverrides.end() ||
           overrides.vec3Overrides.find(propName) != overrides.vec3Overrides.end() ||
           overrides.vec4Overrides.find(propName) != overrides.vec4Overrides.end() ||
           overrides.mat4Overrides.find(propName) != overrides.mat4Overrides.end();
}

void ManagedMeshRenderer::ClearOverride(uint64_t handle, MonoString* propertyName)
{
    ecs::entity target{ handle };


    const char* nativeName = mono_string_to_utf8(propertyName);
    std::string strName = nativeName;


    if (!target.any<MaterialOverridesComponent>()) {
        return;
    }

    auto& overrides = target.get<MaterialOverridesComponent>();
    std::string propName(strName);

    overrides.floatOverrides.erase(propName);
    overrides.vec3Overrides.erase(propName);
    overrides.vec4Overrides.erase(propName);
    overrides.mat4Overrides.erase(propName);
}

void ManagedMeshRenderer::ClearAllOverrides(uint64_t handle)
{
    ecs::entity target{ handle };

    if (!target.any<MaterialOverridesComponent>()) {
        return;
    }

    auto& overrides = target.get<MaterialOverridesComponent>();
    overrides.ClearAll();
}

// ========== Visibility Control ==========

bool ManagedMeshRenderer::GetVisible(uint64_t handle)
{
    ecs::entity target{ handle };

    if (!target.any<VisibilityComponent>()) {
        return true; // Default is visible
    }

    auto& visibility = target.get<VisibilityComponent>();
    return visibility.m_IsVisible;
}

void ManagedMeshRenderer::SetVisible(uint64_t handle, bool visible)
{
    ecs::entity target{ handle };

    // Get or create VisibilityComponent
    if (!target.any<VisibilityComponent>()) {
        target.add<VisibilityComponent>();
    }

    auto& visibility = target.get<VisibilityComponent>();
    visibility.m_IsVisible = visible;
}
