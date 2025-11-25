using System.Runtime.CompilerServices;
using Engine.Bindings;
using BasilEngine.Mathematics;

namespace BasilEngine
{
    [NativeHeader("Bindings/ManagedScreen.hpp")]
    [NativeClass("ManagedScreen")]
    public static class Screen
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetWidth")]
        [StaticAccessor("ManagedScreen", StaticAccessorType.DoubleColon)]
        private static extern int Internal_GetWidth();

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetHeight")]
        [StaticAccessor("ManagedScreen", StaticAccessorType.DoubleColon)]
        private static extern int Internal_GetHeight();

        public static int width => Internal_GetWidth();
        public static int height => Internal_GetHeight();
        public static Vector2 size => new Vector2(width, height);
    }
}
