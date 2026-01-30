using System;
using System.Runtime.CompilerServices;
using BasilEngine.Mathematics;
using Engine.Bindings;

namespace BasilEngine.Components
{
    [NativeHeader("Bindings/ManagedMeshRenderer.hpp")]
    [NativeClass("ManagedMeshRenderer")]

    /// <summary>
    /// MeshRenderer component that exposes material properties for runtime modification.
    /// Provides access to basic PBR material properties and advanced property overrides.
    /// </summary>
    public class MeshRenderer : Component
    {
        // ========== Internal Calls - Basic Material Properties ==========

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetAlbedoColor")]
        [StaticAccessor("ManagedMeshRenderer", StaticAccessorType.DoubleColon)]
        private static extern void Internal_GetAlbedoColor(UInt64 handle, out float r, out float g, out float b);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetAlbedoColor")]
        [StaticAccessor("ManagedMeshRenderer", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetAlbedoColor(UInt64 handle, float r, float g, float b);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetMetallic")]
        [StaticAccessor("ManagedMeshRenderer", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetMetallic(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetMetallic")]
        [StaticAccessor("ManagedMeshRenderer", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetMetallic(UInt64 handle, float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetRoughness")]
        [StaticAccessor("ManagedMeshRenderer", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetRoughness(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetRoughness")]
        [StaticAccessor("ManagedMeshRenderer", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetRoughness(UInt64 handle, float value);

        // ========== Internal Calls - Material Property Overrides ==========

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetFloatOverride")]
        [StaticAccessor("ManagedMeshRenderer", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetFloatOverride(UInt64 handle, string propertyName, float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetVec3Override")]
        [StaticAccessor("ManagedMeshRenderer", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetVec3Override(UInt64 handle, string propertyName, float x, float y, float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetVec4Override")]
        [StaticAccessor("ManagedMeshRenderer", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetVec4Override(UInt64 handle, string propertyName, float x, float y, float z, float w);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("HasOverride")]
        [StaticAccessor("ManagedMeshRenderer", StaticAccessorType.DoubleColon)]
        private static extern bool Internal_HasOverride(UInt64 handle, string propertyName);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("ClearOverride")]
        [StaticAccessor("ManagedMeshRenderer", StaticAccessorType.DoubleColon)]
        private static extern void Internal_ClearOverride(UInt64 handle, string propertyName);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("ClearAllOverrides")]
        [StaticAccessor("ManagedMeshRenderer", StaticAccessorType.DoubleColon)]
        private static extern void Internal_ClearAllOverrides(UInt64 handle);

        // ========== Internal Calls - Visibility ==========

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetVisible")]
        [StaticAccessor("ManagedMeshRenderer", StaticAccessorType.DoubleColon)]
        private static extern bool Internal_GetVisible(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetVisible")]
        [StaticAccessor("ManagedMeshRenderer", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetVisible(UInt64 handle, bool visible);

        // ========== Public Properties - Basic Material ==========

        /// <summary>
        /// Base albedo color of the material (RGB).
        /// This modifies the MeshRendererComponent.material struct directly.
        /// </summary>
        public Vector3 albedoColor
        {
            get
            {
                Internal_GetAlbedoColor(NativeID, out float r, out float g, out float b);
                return new Vector3(r, g, b);
            }
            set => Internal_SetAlbedoColor(NativeID, value.x, value.y, value.z);
        }

        /// <summary>
        /// Metallic value (0.0 = dielectric, 1.0 = metallic).
        /// This modifies the MeshRendererComponent.material struct directly.
        /// </summary>
        public float metallic
        {
            get => Internal_GetMetallic(NativeID);
            set => Internal_SetMetallic(NativeID, value);
        }

        /// <summary>
        /// Surface roughness (0.0 = smooth, 1.0 = rough).
        /// This modifies the MeshRendererComponent.material struct directly.
        /// </summary>
        public float roughness
        {
            get => Internal_GetRoughness(NativeID);
            set => Internal_SetRoughness(NativeID, value);
        }

        /// <summary>
        /// Visibility toggle for this renderer.
        /// </summary>
        public bool visible
        {
            get => Internal_GetVisible(NativeID);
            set => Internal_SetVisible(NativeID, value);
        }

        // ========== Public Methods - Material Property Overrides ==========

        /// <summary>
        /// Set a float material property override.
        /// This creates a MaterialOverridesComponent and preserves GPU instancing.
        /// Use shader uniform names (e.g., "u_MetallicValue", "u_RoughnessValue").
        /// </summary>
        /// <param name="propertyName">Shader uniform name</param>
        /// <param name="value">Float value</param>
        public void SetFloatProperty(string propertyName, float value)
        {
            Internal_SetFloatOverride(NativeID, propertyName, value);
        }

        /// <summary>
        /// Set a Vector3 material property override.
        /// This creates a MaterialOverridesComponent and preserves GPU instancing.
        /// Use shader uniform names (e.g., "u_AlbedoColor", "u_EmissionColor").
        /// </summary>
        /// <param name="propertyName">Shader uniform name</param>
        /// <param name="value">Vector3 value</param>
        public void SetVec3Property(string propertyName, Vector3 value)
        {
            Internal_SetVec3Override(NativeID, propertyName, value.x, value.y, value.z);
        }

        /// <summary>
        /// Set a Vector4 material property override.
        /// This creates a MaterialOverridesComponent and preserves GPU instancing.
        /// Use shader uniform names (e.g., "u_CustomColor").
        /// </summary>
        /// <param name="propertyName">Shader uniform name</param>
        /// <param name="x">X component</param>
        /// <param name="y">Y component</param>
        /// <param name="z">Z component</param>
        /// <param name="w">W component</param>
        public void SetVec4Property(string propertyName, float x, float y, float z, float w)
        {
            Internal_SetVec4Override(NativeID, propertyName, x, y, z, w);
        }

        /// <summary>
        /// Check if a material property override exists for the specified property.
        /// </summary>
        /// <param name="propertyName">Shader uniform name</param>
        /// <returns>True if override exists</returns>
        public bool HasPropertyOverride(string propertyName)
        {
            return Internal_HasOverride(NativeID, propertyName);
        }

        /// <summary>
        /// Clear a specific material property override.
        /// </summary>
        /// <param name="propertyName">Shader uniform name</param>
        public void ClearPropertyOverride(string propertyName)
        {
            Internal_ClearOverride(NativeID, propertyName);
        }

        /// <summary>
        /// Clear all material property overrides.
        /// </summary>
        public void ClearAllPropertyOverrides()
        {
            Internal_ClearAllOverrides(NativeID);
        }
    }
}
