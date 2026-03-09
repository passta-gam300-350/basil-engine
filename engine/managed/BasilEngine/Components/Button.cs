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

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetDisabled")]
        [StaticAccessor("ManagedButton", StaticAccessorType.DoubleColon)]
        private static extern bool GetDisabledInternal(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetDisabled")]
        [StaticAccessor("ManagedButton", StaticAccessorType.DoubleColon)]
        private static extern void SetDisabledInternal(UInt64 handle, bool disabled);

        public bool Hovered => GetHoveredInternal(NativeID); // Expose the hovered state of the button as a read-only property
        public bool Pressed => GetPressedInternal(NativeID); // Expose the pressed state of the button as a read-only property
        public bool Clicked => GetClickedInternal(NativeID); // Expose the clicked state of the button as a read-only property
        public bool Disabled
        {
            get => GetDisabledInternal(NativeID);
            set => SetDisabledInternal(NativeID, value);
        }
    }
}
