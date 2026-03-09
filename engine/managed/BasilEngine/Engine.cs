



using System.Runtime.CompilerServices;
using Engine.Bindings;

namespace BasilEngine
{
    [NativeHeader("Bindings/ManagedEngine.hpp")]
    [NativeClass("ManagedEngine")]
    public class Engine
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetGamma")]
        [StaticAccessor("ManagedEngine", StaticAccessorType.DoubleColon)]
        private extern static float GetGammaInternal();
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetGamma")]
        [StaticAccessor("ManagedEngine", StaticAccessorType.DoubleColon)]
        private extern static void SetGammaInternal(float gamma);

        public static float Gamma
        {
            get => GetGammaInternal();
            set => SetGammaInternal(value);
        }

    }
}
