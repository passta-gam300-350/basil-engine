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


    public class Time
    {
        [NativeMethod("GetDeltaTime")]
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedTime", StaticAccessorType.DoubleColon)]
        private extern static float GetDeltaTimeInternal();



        public static float deltaTime
        {
            get { return GetDeltaTimeInternal(); }
        }

    }
}