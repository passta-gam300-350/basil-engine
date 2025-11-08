using System;
using System.Numerics;
using System.Runtime.CompilerServices;
using Engine.Bindings;

namespace BasilEngine.Components
{
    [NativeHeader("Bindings/ManagedParticle.hpp")]
    public class Particle : Component
    {
        public enum EmissionType { Point = 0, Box = 1, Sphere = 2 }
        public enum BlendMode { Alpha = 0, Additive = 1, Multiply = 2, Opaque = 3 }

        // ---- Playback ----
        public void Play() => Internal_Play(NativeID);
        public void Stop() => Internal_Stop(NativeID);
        public void Reset() => Internal_Reset(NativeID);
        public bool isPlaying => Internal_IsPlaying(NativeID);
        public bool isAlive => Internal_IsAlive(NativeID);
        public int activeCount => Internal_GetActiveCount(NativeID);

        // ---- Component flags ----
        public bool autoPlay { get => Internal_GetAutoPlay(NativeID); set => Internal_SetAutoPlay(NativeID, value); }
        public BlendMode blend { get => (BlendMode)Internal_GetBlendMode(NativeID); set => Internal_SetBlendMode(NativeID, (int)value); }
        public bool depthWrite { get => Internal_GetDepthWrite(NativeID); set => Internal_SetDepthWrite(NativeID, value); }
        public uint renderLayer { get => Internal_GetRenderLayer(NativeID); set => Internal_SetRenderLayer(NativeID, value); }

        // ---- Config: Emission shape ----
        public EmissionType emissionType
        {
            get => (EmissionType)Internal_GetEmissionType(NativeID);
            set => Internal_SetEmissionType(NativeID, (int)value);
        }
        public Vector3 emitterSize
        {
            get => new Vector3(Internal_GetEmitterSizeX(NativeID),
                               Internal_GetEmitterSizeY(NativeID),
                               Internal_GetEmitterSizeZ(NativeID));
            set => Internal_SetEmitterSize(NativeID, value.X, value.Y, value.Z);
        }
        public float sphereRadius
        {
            get => Internal_GetSphereRadius(NativeID);
            set => Internal_SetSphereRadius(NativeID, value);
        }

        // ---- Config: Emission settings ----
        public float emissionRate { get => Internal_GetEmissionRate(NativeID); set => Internal_SetEmissionRate(NativeID, value); }
        public int maxParticles { get => Internal_GetMaxParticles(NativeID); set => Internal_SetMaxParticles(NativeID, value); }
        public bool looping { get => Internal_GetLooping(NativeID); set => Internal_SetLooping(NativeID, value); }
        public float duration { get => Internal_GetDuration(NativeID); set => Internal_SetDuration(NativeID, value); }

        // ---- Config: Lifetime ----
        public float minLifetime { get => Internal_GetMinLifetime(NativeID); set => Internal_SetMinLifetime(NativeID, value); }
        public float maxLifetime { get => Internal_GetMaxLifetime(NativeID); set => Internal_SetMaxLifetime(NativeID, value); }

        // ---- Config: Visuals ----
        public Vector4 startColor
        {
            get => new Vector4(Internal_GetStartColorR(NativeID),
                               Internal_GetStartColorG(NativeID),
                               Internal_GetStartColorB(NativeID),
                               Internal_GetStartColorA(NativeID));
            set => Internal_SetStartColor(NativeID, value.X, value.Y, value.Z, value.W);
        }
        public Vector4 endColor
        {
            get => new Vector4(Internal_GetEndColorR(NativeID),
                               Internal_GetEndColorG(NativeID),
                               Internal_GetEndColorB(NativeID),
                               Internal_GetEndColorA(NativeID));
            set => Internal_SetEndColor(NativeID, value.X, value.Y, value.Z, value.W);
        }
        public float startSize { get => Internal_GetStartSize(NativeID); set => Internal_SetStartSize(NativeID, value); }
        public float endSize { get => Internal_GetEndSize(NativeID); set => Internal_SetEndSize(NativeID, value); }
        public float startRotation { get => Internal_GetStartRotation(NativeID); set => Internal_SetStartRotation(NativeID, value); }
        public float rotationSpeed { get => Internal_GetRotationSpeed(NativeID); set => Internal_SetRotationSpeed(NativeID, value); }

        // ---- Config: Physics ----
        public Vector3 startVelocity
        {
            get => new Vector3(Internal_GetStartVelX(NativeID),
                               Internal_GetStartVelY(NativeID),
                               Internal_GetStartVelZ(NativeID));
            set => Internal_SetStartVelocity(NativeID, value.X, value.Y, value.Z);
        }
        public float velocityRandomness { get => Internal_GetVelocityRandomness(NativeID); set => Internal_SetVelocityRandomness(NativeID, value); }
        public Vector3 acceleration
        {
            get => new Vector3(Internal_GetAccelX(NativeID),
                               Internal_GetAccelY(NativeID),
                               Internal_GetAccelZ(NativeID));
            set => Internal_SetAcceleration(NativeID, value.X, value.Y, value.Z);
        }

        // ===== Native bindings =====
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("Play")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_Play(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("Stop")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_Stop(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("Reset")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_Reset(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("IsPlaying")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern bool Internal_IsPlaying(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("IsAlive")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern bool Internal_IsAlive(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetActiveCount")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern int Internal_GetActiveCount(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetAutoPlay")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern bool Internal_GetAutoPlay(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetAutoPlay")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetAutoPlay(UInt64 handle, bool v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetBlendMode")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern int Internal_GetBlendMode(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetBlendMode")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetBlendMode(UInt64 handle, int v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetDepthWrite")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern bool Internal_GetDepthWrite(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetDepthWrite")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetDepthWrite(UInt64 handle, bool v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetRenderLayer")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern uint Internal_GetRenderLayer(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetRenderLayer")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetRenderLayer(UInt64 handle, uint v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetEmissionType")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern int Internal_GetEmissionType(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetEmissionType")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetEmissionType(UInt64 handle, int v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetEmitterSizeX")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetEmitterSizeX(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetEmitterSizeY")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetEmitterSizeY(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetEmitterSizeZ")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetEmitterSizeZ(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetEmitterSize")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetEmitterSize(UInt64 handle, float x, float y, float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetSphereRadius")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetSphereRadius(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetSphereRadius")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetSphereRadius(UInt64 handle, float r);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetEmissionRate")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetEmissionRate(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetEmissionRate")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetEmissionRate(UInt64 handle, float v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetMaxParticles")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern int Internal_GetMaxParticles(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetMaxParticles")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetMaxParticles(UInt64 handle, int v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetLooping")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern bool Internal_GetLooping(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetLooping")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetLooping(UInt64 handle, bool v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetDuration")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetDuration(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetDuration")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetDuration(UInt64 handle, float v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetMinLifetime")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetMinLifetime(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetMinLifetime")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetMinLifetime(UInt64 handle, float v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetMaxLifetime")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetMaxLifetime(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetMaxLifetime")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetMaxLifetime(UInt64 handle, float v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetStartColorR")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetStartColorR(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetStartColorG")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetStartColorG(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetStartColorB")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetStartColorB(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetStartColorA")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetStartColorA(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetStartColor")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetStartColor(UInt64 handle, float r, float g, float b, float a);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetEndColorR")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetEndColorR(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetEndColorG")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetEndColorG(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetEndColorB")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetEndColorB(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetEndColorA")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetEndColorA(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetEndColor")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetEndColor(UInt64 handle, float r, float g, float b, float a);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetStartSize")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetStartSize(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetStartSize")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetStartSize(UInt64 handle, float v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetEndSize")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetEndSize(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetEndSize")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetEndSize(UInt64 handle, float v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetStartRotation")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetStartRotation(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetStartRotation")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetStartRotation(UInt64 handle, float v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetRotationSpeed")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetRotationSpeed(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetRotationSpeed")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetRotationSpeed(UInt64 handle, float v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetStartVelX")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetStartVelX(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetStartVelY")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetStartVelY(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetStartVelZ")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetStartVelZ(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetStartVelocity")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetStartVelocity(UInt64 handle, float x, float y, float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetVelocityRandomness")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetVelocityRandomness(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetVelocityRandomness")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetVelocityRandomness(UInt64 handle, float v);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetAccelX")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetAccelX(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetAccelY")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetAccelY(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetAccelZ")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern float Internal_GetAccelZ(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetAcceleration")]
        [StaticAccessor("ManagedParticle", StaticAccessorType.DoubleColon)]
        private static extern void Internal_SetAcceleration(UInt64 handle, float x, float y, float z);
    }
}
