using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using BasilEngine.Mathematics;
using Engine.Bindings;

namespace BasilEngine.Components
{
    [NativeHeader("Bindings/ManagedCamera.hpp")]
    [NativeClass("ManagedCamera")]

    public class Camera : Component
    {
   
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetCameraType")]
        [StaticAccessor("ManagedCamera", StaticAccessorType.DoubleColon)]
        public static extern void Internal_SetType(UInt64 handle, int type);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetCameraType")]
        [StaticAccessor("ManagedCamera", StaticAccessorType.DoubleColon)]
        public static extern int Internal_GetType(UInt64 handle);

        //[MethodImpl(MethodImplOptions.InternalCall)]
        //public static extern void Internal_SetActive(UInt64 handle, bool active);
        //[MethodImpl(MethodImplOptions.InternalCall)]
        //public static extern bool Internal_GetActive(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("setFoV")]
        [StaticAccessor("ManagedCamera", StaticAccessorType.DoubleColon)]
        public static extern void Internal_SetFOV(UInt64 handle, float fov);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("getFoV")]
        [StaticAccessor("ManagedCamera", StaticAccessorType.DoubleColon)]
        public static extern float Internal_GetFOV(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("setAspectRatio")]
        [StaticAccessor("ManagedCamera", StaticAccessorType.DoubleColon)]
        public static extern void Internal_SetAspectRatio(UInt64 handle, float ratio);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("getAspectRatio")]
        [StaticAccessor("ManagedCamera", StaticAccessorType.DoubleColon)]

        public static extern float Internal_GetAspectRatio(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("setNear")]
        [StaticAccessor("ManagedCamera", StaticAccessorType.DoubleColon)]
        public static extern void Internal_SetNearPlane(UInt64 handle, float near);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("setFar")]
        [StaticAccessor("ManagedCamera", StaticAccessorType.DoubleColon)]
        public static extern void Internal_SetFarPlane(UInt64 handle, float far);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("getNear")]
        [StaticAccessor("ManagedCamera", StaticAccessorType.DoubleColon)]

        public static extern float Internal_GetNearPlane(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("getFar")]
        [StaticAccessor("ManagedCamera", StaticAccessorType.DoubleColon)]
        public static extern float Internal_GetFarPlane(UInt64 handle);

        public enum CameraType
        {
            ORTHOGRAPHIC = 0,
            PERSPECTIVE = 1
        }

        public bool active
        {
            get => false;
        }

        public CameraType type
        {
            get => (CameraType)Internal_GetType(NativeID);
            set => Internal_SetType(NativeID, (int)value);
        }

        public float fov
        {
            get => Internal_GetFOV(NativeID);
            set => Internal_SetFOV(NativeID, value);
        }
        public float AspectRatio
        {
            get => Internal_GetAspectRatio(NativeID);
            set => Internal_SetAspectRatio(NativeID, value);
        }

        public float near
        {
            get => Internal_GetNearPlane(NativeID);
            set => Internal_SetNearPlane(NativeID, value);

        }

        public float far
        {
            get => Internal_GetFarPlane(NativeID);
            set => Internal_SetFarPlane(NativeID, value);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("ScreenToWorldPoint")]
        [StaticAccessor("ManagedCamera", StaticAccessorType.DoubleColon)]
        private static extern void Internal_ScreenToWorldPoint(UInt64 handle, float x, float y, float depth, out float wx, out float wy, out float wz);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("ScreenPointToRay")]
        [StaticAccessor("ManagedCamera", StaticAccessorType.DoubleColon)]
        private static extern void Internal_ScreenPointToRay(UInt64 handle, float x, float y, float _unusedDepth,
                                                             out float ox, out float oy, out float oz,
                                                             out float dx, out float dy, out float dz);

        public Vector3 ScreenToWorldPoint(Vector2 screenPoint, float depth=0)
        {
            // Treat depth as world-space distance from the near plane (depth=0 => near plane)
            float distRange = far - near;
            float depthClamped = depth < 0 ? 0f : depth;
            float normalizedDepth = distRange > 0 ? depthClamped / distRange : 0f;
            Internal_ScreenToWorldPoint(NativeID, screenPoint.x, screenPoint.y, normalizedDepth, out var wx, out var wy, out var wz);
            return new Vector3(wx, wy, wz);
        }

        public Ray ScreenPointToRay(Vector2 screenPoint)
        {
            Internal_ScreenPointToRay(NativeID, screenPoint.x, screenPoint.y, 0f,
                                      out var ox, out var oy, out var oz,
                                      out var dx, out var dy, out var dz);
            return new Ray(new Vector3(ox, oy, oz), new Vector3(dx, dy, dz));
        }

    }
}
