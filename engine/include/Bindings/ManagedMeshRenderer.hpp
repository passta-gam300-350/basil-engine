/******************************************************************************/
/*!
\file   ManagedMeshRenderer.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/30
\brief  This file contains the declaration for the ManagedMeshRenderer class, which
        is responsible for managing mesh renderer and material-related functionalities
        in the managed environment.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#ifndef MANAGEDMESHRENDERER_HPP
#define MANAGEDMESHRENDERER_HPP
#include <cstdint>
using MonoString = struct _MonoString;
class ManagedMeshRenderer
{
public:
    // ========== Basic Material Properties (MeshRendererComponent.material) ==========

    /// Get albedo color (RGB) from MeshRendererComponent.material
    static void GetAlbedoColor(uint64_t handle, float* r, float* g, float* b);

    /// Set albedo color (RGB) in MeshRendererComponent.material
    static void SetAlbedoColor(uint64_t handle, float r, float g, float b);

    /// Get metallic value from MeshRendererComponent.material
    static float GetMetallic(uint64_t handle);

    /// Set metallic value in MeshRendererComponent.material
    static void SetMetallic(uint64_t handle, float value);

    /// Get roughness value from MeshRendererComponent.material
    static float GetRoughness(uint64_t handle);

    /// Set roughness value in MeshRendererComponent.material
    static void SetRoughness(uint64_t handle, float value);

    // ========== Material Property Overrides (MaterialOverridesComponent) ==========

    /// Set a float property override (creates MaterialOverridesComponent if needed)
    static void SetFloatOverride(uint64_t handle, MonoString* propertyName, float value);

    /// Set a vec3 property override (creates MaterialOverridesComponent if needed)
    static void SetVec3Override(uint64_t handle, MonoString* propertyName, float x, float y, float z);

    /// Set a vec4 property override (creates MaterialOverridesComponent if needed)
    static void SetVec4Override(uint64_t handle, MonoString* propertyName, float x, float y, float z, float w);

    /// Check if a property override exists
    static bool HasOverride(uint64_t handle, MonoString* propertyName);

    /// Clear a specific property override
    static void ClearOverride(uint64_t handle, MonoString* propertyName);

    /// Clear all property overrides
    static void ClearAllOverrides(uint64_t handle);

    // ========== Visibility Control ==========

    /// Get entity visibility state
    static bool GetVisible(uint64_t handle);

    /// Set entity visibility state (creates VisibilityComponent if needed)
    static void SetVisible(uint64_t handle, bool visible);
};

#endif // MANAGEDMESHRENDERER_HPP
