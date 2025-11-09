using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using Engine.Bindings;

namespace BasilEngine.Components
{
    [NativeHeader("Bindings/ManagedAudio.hpp")]
    [NativeClass("ManagedAudio")]

    public class Audio : Component
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetVolume")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        public static extern void setVolume(UInt64 handle, float volume);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetVolume")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        public static extern float getVolume(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetIsLooping")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        public static extern void SetIsLooping(UInt64 handle, bool isLooping);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetIsLooping")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        public static extern bool GetIsLooping(UInt64 handle);

        //[MethodImpl(MethodImplOptions.InternalCall)]
        //public static extern void SetPlayOnAwake(UInt64 handle, bool playOnAwake);

        //[MethodImpl(MethodImplOptions.InternalCall)]
        //public static extern bool GetPlayOnAwake(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("Play")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        public static extern void InternalPlay(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("Pause")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        public static extern void InternalPause(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("Stop")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        public static extern void InternalStop(UInt64 handle);


        public float Volume
        {
            get => getVolume(NativeID);
            set => setVolume(NativeID, value);
        }

        public bool Looping
        {
            get => GetIsLooping(NativeID);
            set => SetIsLooping(NativeID, value);
        }


        public void Play()
        {
            InternalPlay(NativeID);
        }

        public void Pause()
        {
            InternalPause(NativeID);
        }
        public void Stop()
        {
            InternalStop(NativeID);
        }
    }
}
