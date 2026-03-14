using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using Engine.Bindings;

namespace BasilEngine.Components
{
    [NativeHeader("Bindings/ManagedAnimation.hpp")]
    [NativeClass("ManagedAnimation")]
    /// <summary>
    /// Animation component providing skeletal animation controls.
    /// </summary>
    public class Animation : Component
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedAnimation", StaticAccessorType.DoubleColon)]
        internal static extern void Play(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedAnimation", StaticAccessorType.DoubleColon)]
        internal static extern void Pause(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedAnimation", StaticAccessorType.DoubleColon)]
        internal static extern void Stop(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedAnimation", StaticAccessorType.DoubleColon)]
        internal static extern void SetLoop(UInt64 handle, bool loop);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedAnimation", StaticAccessorType.DoubleColon)]
        internal static extern void SetPlaybackSpeed(UInt64 handle, float speed);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedAnimation", StaticAccessorType.DoubleColon)]
        internal static extern bool IsAnimationFinished(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedAnimation", StaticAccessorType.DoubleColon)]
        internal static extern bool PlayAnimation(UInt64 handle, string animationName, bool shouldLoop);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedAnimation", StaticAccessorType.DoubleColon)]
        internal static extern bool HasAnimation(UInt64 handle, string animationName);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedAnimation", StaticAccessorType.DoubleColon)]
        internal static extern void SetSpritesheetMode(UInt64 handle, bool enabled);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedAnimation", StaticAccessorType.DoubleColon)]
        internal static extern bool GetSpritesheetMode(UInt64 handle);

        /// <summary>
        /// Resumes playing the current animation.
        /// </summary>
        public void Play() => Play(NativeID);
        
        /// <summary>
        /// Pauses the current animation.
        /// </summary>
        public void Pause() => Pause(NativeID);
        
        /// <summary>
        /// Stops the current animation and resets it to the start.
        /// </summary>
        public void Stop() => Stop(NativeID);

        /// <summary>
        /// Whether the animation should loop.
        /// </summary>
        public bool Loop
        {
            set => SetLoop(NativeID, value);
        }

        /// <summary>
        /// Speed multiplier for animation playback.
        /// </summary>
        public float PlaybackSpeed
        {
            set => SetPlaybackSpeed(NativeID, value);
        }

        /// <summary>
        /// Whether the current non-looping animation has finished playing.
        /// </summary>
        public bool IsFinished => IsAnimationFinished(NativeID);

        /// <summary>
        /// Plays an animation by name.
        /// </summary>
        /// <param name="animationName">Name of the animation clip.</param>
        /// <param name="shouldLoop">Whether the animation should loop.</param>
        /// <returns>True if the animation was found and started playing.</returns>
        public bool Play(string animationName, bool shouldLoop = true)
        {
            return PlayAnimation(NativeID, animationName, shouldLoop);
        }

        /// <summary>
        /// Checks if an animation clip exists by name.
        /// </summary>
        /// <param name="animationName">Name of the animation clip.</param>
        /// <returns>True if found.</returns>
        public bool HasAnimation(string animationName)
        {
            return HasAnimation(NativeID, animationName);
        }

        /// <summary>
        /// Enables or disables spritesheet flipbook mode.
        /// When enabled, bones that displace vertices in Y are used to show/hide sprite rows.
        /// </summary>
        public bool SpritesheetMode
        {
            get => GetSpritesheetMode(NativeID);
            set => SetSpritesheetMode(NativeID, value);
        }
    }
}
