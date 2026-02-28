using System;
using System.Runtime.CompilerServices;
using Engine.Bindings;

namespace BasilEngine.Components
{
    [NativeHeader("Bindings/ManagedButton.hpp")]
    [NativeClass("ManagedButton")]
    public class Button : Component
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetHovered")]
        [StaticAccessor("ManagedButton", StaticAccessorType.DoubleColon)]
        private static extern bool GetHoveredInternal(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetPressed")]
        [StaticAccessor("ManagedButton", StaticAccessorType.DoubleColon)]
        private static extern bool GetPressedInternal(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetClicked")]
        [StaticAccessor("ManagedButton", StaticAccessorType.DoubleColon)]
        private static extern bool GetClickedInternal(UInt64 handle);

        public bool Hovered => GetHoveredInternal(NativeID);
        public bool Pressed => GetPressedInternal(NativeID);
        public bool Clicked => GetClickedInternal(NativeID);
    }
}
