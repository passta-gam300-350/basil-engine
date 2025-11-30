using Engine.Bindings;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace BasilEngine.Components
{
    [NativeHeader("Bindings/ManagedVideo.hpp")]
    [NativeClass("ManagedVideo")]

    public class Video : Component
    {

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetIsPlaying")]
        [StaticAccessor("ManagedVideo", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Gets if the video is playing.
        /// </summary>
        /// <param name="handle">Native video handle.</param>
        /// <returns>True if the video is playing, false otherwise.</returns>
        public static extern bool GetIsPlayingInternal(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetPlaying")]
        [StaticAccessor("ManagedVideo", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Gets if the video is playing.
        /// </summary>
        /// <param name="handle">Native video handle.</param>
        /// <returns>True if the video is playing, false otherwise.</returns>
        public static extern void SetPlayingInternal(UInt64 handle, bool value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetIsLooping")]
        [StaticAccessor("ManagedVideo", StaticAccessorType.DoubleColon)]
        public static extern void SetLoopInternal(UInt64 handle, bool value);
        public bool isPlaying
        {
            get => GetIsPlayingInternal(NativeID);
            set => SetPlayingInternal(NativeID, value);
        }

        public bool looping
        {
            set => SetLoopInternal(NativeID, value);
        }
    }
}

