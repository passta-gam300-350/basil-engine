using System;
using System.Numerics;
using System.Runtime.CompilerServices;
using Engine.Bindings;

namespace BasilEngine.Components
{
    [NativeHeader("Bindings/ManagedLight.h")]
    public class Light : Component
    {
        public enum LightType { Directional = 0, Point = 1, Spot = 2 }

        // -------- Type --------
        public LightType type
        {
            get => (LightType)Internal_GetType(NativeID);
            set => Internal_SetType(NativeID, (int)value);
        }

        // -------- Enabled --------
        public bool enabled
        {
            get => Internal_GetEnabled(NativeID);
            set => Internal_SetEnabled(NativeID, value);
        }

        // -------- Color --------
        public Vector3 color
        {
            get => new Vector3(colorR, colorG, colorB);
            set => Internal_SetColor(NativeID, value.X, value.Y, value.Z);
        }
        public float colorR { get => Internal_GetColorR(NativeID); set => Internal_SetColorR(NativeID, value); }
        public float colorG { get => Internal_GetColorG(NativeID); set => Internal_SetColorG(NativeID, value); }
        public float colorB { get => Internal_GetColorB(NativeID); set => Internal_SetColorB(NativeID, value); }

        // -------- Intensity --------
        public float intensity
        {
            get => Internal_GetIntensity(NativeID);
            set => Internal_SetIntensity(NativeID, value);
        }

        // -------- Direction --------
        public Vector3 direction
        {
            get => new Vector3(dirX, dirY, dirZ);
            set => Internal_SetDirection(NativeID, value.X, value.Y, value.Z);
        }
        public float dirX => Internal_GetDirectionX(NativeID);
        public float dirY => Internal_GetDirectionY(NativeID);
        public float dirZ => Internal_GetDirectionZ(NativeID);

        // -------- Range --------
        public float range
        {
            get => Internal_GetRange(NativeID);
            set => Internal_SetRange(NativeID, value);
        }

        // -------- Spot cones (degrees) --------
        public float innerConeDeg
        {
            get => Internal_GetInnerConeDeg(NativeID);
            set => Internal_SetCones(NativeID, value, outerConeDeg);
        }
        public float outerConeDeg
        {
            get => Internal_GetOuterConeDeg(NativeID);
            set => Internal_SetCones(NativeID, innerConeDeg, value);
        }
        public void SetCones(float innerDeg, float outerDeg) => Internal_SetCones(NativeID, innerDeg, outerDeg);

        // ===== Native bindings =====
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetLightType")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetType(UInt64 handle, int type);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetLightType")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern int Internal_GetType(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetEnabled")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern bool Internal_GetEnabled(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetEnabled")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetEnabled(UInt64 handle, bool v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetColorR")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetColorR(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetColorG")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetColorG(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetColorB")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetColorB(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetColor")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetColor(UInt64 handle, float r, float g, float b);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetColorR")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetColorR(UInt64 handle, float r);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetColorG")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetColorG(UInt64 handle, float g);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetColorB")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetColorB(UInt64 handle, float b);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetIntensity")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetIntensity(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetIntensity")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetIntensity(UInt64 handle, float v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetDirectionX")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetDirectionX(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetDirectionY")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetDirectionY(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetDirectionZ")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetDirectionZ(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetDirection")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetDirection(UInt64 handle, float x, float y, float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetRange")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetRange(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetRange")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetRange(UInt64 handle, float v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetInnerConeDeg")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetInnerConeDeg(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetOuterConeDeg")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetOuterConeDeg(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetCones")]
        [StaticAccessor("ManagedLight", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetCones(UInt64 handle, float innerDeg, float outerDeg);
    }
}
