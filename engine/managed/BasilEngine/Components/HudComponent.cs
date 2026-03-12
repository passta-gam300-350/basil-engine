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
    //[NativeHeader("Bindings/ManagedCamera.hpp")]
    //[NativeClass("ManagedCamera")]
    [NativeHeader("Bindings/ManagedHudComponent.hpp")]
    [NativeClass("ManagedHudComponent")]
    public class HudComponent : Component
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetVisibility")]
        [StaticAccessor("ManagedHudComponent", StaticAccessorType.DoubleColon)]
        public static extern void SetVisibleInternal(UInt64 handle, bool visibility);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetVisibility")]
        [StaticAccessor("ManagedHudComponent", StaticAccessorType.DoubleColon)]
        public static extern bool GetVisibleInternal(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetPosition")]
        [StaticAccessor("ManagedHudComponent", StaticAccessorType.DoubleColon)]
        public static extern void SetPositionInternal(UInt64 handle, float x, float y);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetPosition")]
        [StaticAccessor("ManagedHudComponent", StaticAccessorType.DoubleColon)]
        public static extern void GetPositionInternal(UInt64 handle, out float x, out float y);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetSize")]
        [StaticAccessor("ManagedHudComponent", StaticAccessorType.DoubleColon)]
        public static extern void GetSizeInternal(UInt64 handle, out float width, out float height);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetSize")]
        [StaticAccessor("ManagedHudComponent", StaticAccessorType.DoubleColon)]
        public static extern void SetSizeInternal(UInt64 handle, float width, float height);

        public bool Visible
        {
            get => GetVisibleInternal(NativeID);
            set => SetVisibleInternal(NativeID, value);
        }
        public Vector2 position
        {
            get
            {
                GetPositionInternal(NativeID, out float x, out float y);
                return new Vector2(x, y);
            }
            set => SetPositionInternal(NativeID, value.x, value.y);
        }

        public Vector2 size
        {
            get
            {
                GetSizeInternal(NativeID, out float width, out float height);
                return new Vector2(width, height);
            }
            set => SetSizeInternal(NativeID, value.x, value.y);
        }


    }
}
