
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
    [NativeHeader("Bindings/ManagedWorldUI.hpp")]
    [NativeClass("ManagedWorldUI")]
    public class WorldUI : Component
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetScale")]
        [StaticAccessor("ManagedWorldUI", StaticAccessorType.DoubleColon)]
        private static extern void SetScaleInternal(UInt64 handle, float x, float y);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetScale")]
        [StaticAccessor("ManagedWorldUI", StaticAccessorType.DoubleColon)]
        private static extern void GetScaleInternal(UInt64 handle, out float x, out float y);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetVisibility")]
        [StaticAccessor("ManagedWorldUI", StaticAccessorType.DoubleColon)]
        private static extern void SetVisibleInternal(UInt64 handle, bool visibility);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetVisibility")]
        [StaticAccessor("ManagedWorldUI", StaticAccessorType.DoubleColon)]
        private static extern bool GetVisibleInternal(UInt64 handle);

        public Vector2 Size
        {
            get {
                GetScaleInternal(NativeID, out float x, out float y);
                return new Vector2(x, y);
            }
            set => SetScaleInternal(NativeID, value.x, value.y);
        }

        public bool Visible
        {
            get => GetVisibleInternal(NativeID);
            set => SetVisibleInternal(NativeID, value);
        }




    }
}
