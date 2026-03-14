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

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("AdjustChannelVolume")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Adjusts the volume of a mix channel by a percentage (e.g. +10 = increase by 10%, -20 = decrease by 20%).
        /// </summary>
        /// <param name="channel">Mix group (0=Master, 1=BGM, 2=SFX, 3=UI, 4=Ambient).</param>
        /// <param name="percentDelta">Percent to add; positive = increase, negative = decrease.</param>
        private static extern void AdjustChannelVolume(byte channel, float percentDelta);

        /// <summary>
        /// Adjusts the volume of a mix channel by a percentage (e.g. +10 = increase by 10%, -20 = decrease by 20%).
        /// </summary>
        /// <param name="channel">Mix group (Master, BGM, SFX, UI, Ambient).</param>
        /// <param name="percentDelta">Percent to add; positive = increase, negative = decrease.</param>
        public static void AdjustChannelVolume(AudioChannel channel, float percentDelta)
        {
            AdjustChannelVolume((byte)channel, percentDelta);
        }

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

        /// <summary>
        /// Mix channel (group) for global volume control. Matches native AudioGroup.
        /// </summary>
        public enum AudioChannel : byte
        {
            Master = 0,
            BGM,
            SFX,
            UI,
            Ambient
        }

        /// <summary>
        /// Filter type for per-channel DSP: None, Lowpass, Highpass, Echo.
        /// Backed by int to simplify native interop.
        /// </summary>
        public enum FilterType
        {
            None = 0,
            Lowpass,
            Highpass,
            Echo
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetFilterType")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        private static extern void SetFilterType(UInt64 handle, int filterType);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetFilterType")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        private static extern int GetFilterType(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetFilterCutoff")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        private static extern void SetFilterCutoff(UInt64 handle, float cutoffHz);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetFilterCutoff")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        private static extern float GetFilterCutoff(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetFilterResonance")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        private static extern void SetFilterResonance(UInt64 handle, float resonance);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetFilterResonance")]
        [StaticAccessor("ManagedAudio", StaticAccessorType.DoubleColon)]
        private static extern float GetFilterResonance(UInt64 handle);

        /// <summary>
        /// Filter type (None, Lowpass, Highpass, Echo). Applied when playing.
        /// </summary>
        public FilterType Filter
        {
            get => (FilterType)GetFilterType(NativeID);
            set => SetFilterType(NativeID, (int)value);
        }

        /// <summary>
        /// Filter cutoff frequency in Hz (10–22050). Used when a filter is active.
        /// </summary>
        public float FilterCutoffHz
        {
            get => GetFilterCutoff(NativeID);
            set => SetFilterCutoff(NativeID, value);
        }

        /// <summary>
        /// Filter resonance (0.5–10). Used for Lowpass/Highpass filters.
        /// </summary>
        public float FilterResonance
        {
            get => GetFilterResonance(NativeID);
            set => SetFilterResonance(NativeID, value);
        }
    }
}
