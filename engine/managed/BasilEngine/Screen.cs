using System.Runtime.CompilerServices;
using Engine.Bindings;
using BasilEngine.Mathematics;

namespace BasilEngine
{
    [NativeHeader("Bindings/ManagedScreen.hpp")]
    [NativeClass("ManagedScreen")]
    /// <summary>
    /// Provides access to current screen size reported by the engine.
    /// </summary>
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

        /// <summary>
        /// Width of the current screen in pixels.
        /// </summary>
        public static int width => Internal_GetWidth();
        /// <summary>
        /// Height of the current screen in pixels.
        /// </summary>
        public static int height => Internal_GetHeight();
        /// <summary>
        /// Combined screen size as a <see cref="Mathematics.Vector2"/>.
        /// </summary>
        public static Vector2 size => new Vector2(width, height);
    }
}
