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

    /// <summary>
    /// Component wrapper for native audio playback.
    /// </summary>
    public class Audio : Component
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetVolume")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Sets the playback volume on the native audio source.
        /// </summary>
        /// <param name="handle">Native audio handle.</param>
        /// <param name="volume">Volume multiplier.</param>
        public static extern void setVolume(UInt64 handle, float volume);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetVolume")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Gets the playback volume from the native audio source.
        /// </summary>
        /// <param name="handle">Native audio handle.</param>
        /// <returns>Volume multiplier.</returns>
        public static extern float getVolume(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetIsLooping")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Sets whether the clip should loop.
        /// </summary>
        /// <param name="handle">Native audio handle.</param>
        /// <param name="isLooping">True to loop.</param>
        public static extern void SetIsLooping(UInt64 handle, bool isLooping);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetIsLooping")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Gets whether the clip loops.
        /// </summary>
        /// <param name="handle">Native audio handle.</param>
        /// <returns>True if looping.</returns>
        public static extern bool GetIsLooping(UInt64 handle);

        //[MethodImpl(MethodImplOptions.InternalCall)]
        //public static extern void SetPlayOnAwake(UInt64 handle, bool playOnAwake);

        //[MethodImpl(MethodImplOptions.InternalCall)]
        //public static extern bool GetPlayOnAwake(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("Play")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Starts playback on the native audio source.
        /// </summary>
        /// <param name="handle">Native audio handle.</param>
        public static extern void InternalPlay(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("Pause")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Pauses playback on the native audio source.
        /// </summary>
        /// <param name="handle">Native audio handle.</param>
        public static extern void InternalPause(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("Stop")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Stops playback on the native audio source.
        /// </summary>
        /// <param name="handle">Native audio handle.</param>
        public static extern void InternalStop(UInt64 handle);


        /// <summary>
        /// Volume multiplier for this audio source.
        /// </summary>
        public float Volume
        {
            get => getVolume(NativeID);
            set => setVolume(NativeID, value);
        }

        /// <summary>
        /// Whether the audio should loop when it reaches the end.
        /// </summary>
        public bool Looping
        {
            get => GetIsLooping(NativeID);
            set => SetIsLooping(NativeID, value);
        }


        /// <summary>
        /// Starts playback.
        /// </summary>
        public void Play()
        {
            InternalPlay(NativeID);
        }

        /// <summary>
        /// Pauses playback.
        /// </summary>
        public void Pause()
        {
            InternalPause(NativeID);
        }
        /// <summary>
        /// Stops playback and resets the cursor to the start.
        /// </summary>
        public void Stop()
        {
            InternalStop(NativeID);
        }
    }
}
