using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using Engine.Bindings;

namespace BasilEngine
{
    [NativeHeader("Bindings/ManagedTime.hpp")]
    [NativeClass("ManagedTime")]


    /// <summary>
    /// Exposes time-related values sourced from the engine runtime.
    /// </summary>
    public class Time
    {
        [NativeMethod("GetDeltaTime")]
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedTime", StaticAccessorType.DoubleColon)]
        private extern static float GetDeltaTimeInternal();



        /// <summary>
        /// Time in seconds it took to complete the last frame.
        /// </summary>
        public static float deltaTime
        {
            get { return GetDeltaTimeInternal(); }
        }

    }
}
