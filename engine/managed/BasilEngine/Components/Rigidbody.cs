using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using BasilEngine.Mathematics;
using Engine.Bindings;

namespace BasilEngine.Components
{
    [Disabled]
    /// <summary>
    /// Modes describing how a force is applied to a rigidbody.
    /// </summary>
    public enum ForceMode
    {
        /// <summary>
        /// Continuous force considering mass and timestep.
        /// </summary>
        Force,
        /// <summary>
        /// Instant impulse applied at once.
        /// </summary>
        Impulse,
        /// <summary>
        /// Continuous acceleration ignoring mass.
        /// </summary>
        Acceleration,
        /// <summary>
        /// Instant velocity change ignoring mass.
        /// </summary>
        VelocityChange
    }

    [NativeHeader("Bindings/ManagedPhysics.hpp")]
    [NativeClass("ManagedPhysics")]
    /// <summary>
    /// Physics component providing rigid body controls.
    /// </summary>
    public class Rigidbody : Component
    {
        /// <summary>
        /// Describes how the rigidbody is simulated.
        /// </summary>
        public enum MotionType
        {
            /// <summary>
            /// Body does not move.
            /// </summary>
            Static = 0,
            /// <summary>
            /// Body is fully simulated by physics.
            /// </summary>
            Dynamic = 1,
            /// <summary>
            /// Body is moved kinematically by user code.
            /// </summary>
            Kinematic = 2
        }



        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetMotionType(UInt64 handle, int motionType);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern int GetMotionType(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetMass(UInt64 handle, float mass);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern float GetMass(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetDrag(UInt64 handle, float drag);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern float GetDrag(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetFriction(UInt64 handle, float friction);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern float GetFriction(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetAngularDrag(UInt64 handle, float angularDrag);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern float GetAngularDrag(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetGravityFactor(UInt64 handle, float useAutoGravity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern float GetGravityFactor(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetUseGravity(UInt64 handle, bool useGravity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern bool GetUseGravity(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetIsKinematic(UInt64 handle, bool isKinematic);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern bool GetIsKinematic(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetFreezeX(UInt64 handle, bool freezeX);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern bool GetFreezeX(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetFreezeY(UInt64 handle, bool freezeY);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern bool GetFreezeY(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetFreezeZ(UInt64 handle, bool freezeZ);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern bool GetFreezeZ(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetFreezeRotationX(UInt64 handle, bool freezeRotationX);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern bool GetFreezeRotationX(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetFreezeRotationY(UInt64 handle, bool freezeRotationY);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern bool GetFreezeRotationY(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetFreezeRotationZ(UInt64 handle, bool freezeRotationZ);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern bool GetFreezeRotationZ(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetLinearDamping(UInt64 handle, float linearDamping);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern float GetLinearDamping(UInt64 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetCenterOfMass(UInt64 handle, float x, float y, float z);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void GetCenterOfMass(UInt64 handle, out float x, out float y, out float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetLinearVelocity(UInt64 handle, float x, float y, float z);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void GetLinearVelocity(UInt64 handle, out float x, out float y, out float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void SetAngularVelocity(UInt64 handle, float x, float y, float z);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void GetAngularVelocity(UInt64 handle, out float x, out float y, out float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void AddForceInternal(UInt64 handle, float x, float y, float z);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        public static extern void AddImpulseInternal(UInt64 handle, float x, float y, float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        [NativeMethod("MovePosition")]
        public static extern void MovePositionInternal(UInt64 handle, float x, float y, float z);

        /// <summary>
        /// Gets or sets the motion type.
        /// </summary>
        public MotionType motionType
        {
            get => (MotionType)GetMotionType(NativeID);
            set => SetMotionType(NativeID, (int)value);
        }

        /// <summary>
        /// Linear velocity of the rigidbody.
        /// </summary>
        public Vector3 velocity
        {
            get
            {
                GetLinearVelocity(NativeID, out float x, out float y, out float z);
                return new Vector3(x, y, z);
            }
            set
            {
                SetLinearVelocity(NativeID, value.x, value.y, value.z);
            }
        }

        /// <summary>
        /// Gravity multiplier applied to this body.
        /// </summary>
        public float Gravity
        {
            set => SetGravityFactor(NativeID, value);
            get => GetGravityFactor(NativeID);
        }

        /// <summary>
        /// Whether the body is kinematic.
        /// </summary>
        public bool IsKinematic
        {
            get => GetIsKinematic(NativeID);
            set => SetIsKinematic(NativeID, value);
        }
        /// <summary>
        /// Whether gravity affects the body.
        /// </summary>
        public bool UseGravity
        {
            get => GetUseGravity(NativeID);
            set => SetUseGravity(NativeID, value);
        }



        /// <summary>
        /// Mass of the rigidbody.
        /// </summary>
        public float Mass
        {
            get => GetMass(NativeID);
            set => SetMass(NativeID, value);
        }
        /// <summary>
        /// Linear drag coefficient.
        /// </summary>
        public float Drag
        {
            get => GetDrag(NativeID);
            set => SetDrag(NativeID, value);
        }

        /// <summary>
        /// Angular drag coefficient.
        /// </summary>
        public float AngularDrag
        {
            get => GetAngularDrag(NativeID);
            set => SetAngularDrag(NativeID, value);
        }

        /// <summary>
        /// Surface friction coefficient.
        /// </summary>
        public float Friction
        {
            get => GetFriction(NativeID);
            set => SetFriction(NativeID, value);
        }


        /// <summary>
        /// Locks translation on the X axis.
        /// </summary>
        public bool FreezeX
        {
            set => SetFreezeX(NativeID, value);
            get => GetFreezeX(NativeID);
        }

        /// <summary>
        /// Locks translation on the Y axis.
        /// </summary>
        public bool FreezeY
        {
            set => SetFreezeY(NativeID, value);
            get => GetFreezeY(NativeID);
        }

        /// <summary>
        /// Locks translation on the Z axis.
        /// </summary>
        public bool FreezeZ
        {
            set => SetFreezeZ(NativeID, value);
            get => GetFreezeZ(NativeID);
        }

        /// <summary>
        /// Locks rotation around the X axis.
        /// </summary>
        public bool FreezeRotX
        {
            get => GetFreezeRotationX(NativeID);
            set => SetFreezeRotationX(NativeID, value);
        }

        /// <summary>
        /// Locks rotation around the Y axis.
        /// </summary>
        public bool FreezeRotY
        {
            get => GetFreezeRotationY(NativeID);
            set => SetFreezeRotationY(NativeID, value);
        }

        /// <summary>
        /// Locks rotation around the Z axis.
        /// </summary>
        public bool FreezeRotZ
        {
            get => GetFreezeRotationZ(NativeID);
            set => SetFreezeRotationZ(NativeID, value);
        }

        /// <summary>
        /// Linear damping applied each update.
        /// </summary>
        public float LinearDamping
        {
            get => GetLinearDamping(NativeID);
            set => SetLinearDamping(NativeID, value);
        }

        /// <summary>
        /// Angular velocity of the rigidbody.
        /// </summary>
        public Vector3 AngularVelocity
        {
            get
            {
                GetAngularVelocity(NativeID, out float x, out float y, out float z);
                return new Vector3(x, y, z);
            }
            set => SetAngularVelocity(NativeID, value.x, value.y, value.z);
        }

        /// <summary>
        /// Moves the rigidbody to a position using interpolation.
        /// </summary>
        /// <param name="position">Target world position.</param>
        public void MovePosition(Vector3 position)
        {
            MovePositionInternal(NativeID, position.x, position.y, position.z);
        }


        /// <summary>
        /// Applies a force to the rigidbody.
        /// </summary>
        /// <param name="force">Force vector.</param>
        /// <param name="mode">Force application mode.</param>
        public void AddForce(Vector3 force, ForceMode mode)
        {
            switch (mode)
            {
                case ForceMode.Force:
                    AddForceInternal(NativeID, force.x, force.y, force.z);
                    break;
                case ForceMode.Impulse:
                    AddImpulseInternal(NativeID, force.x, force.y, force.z);
                    break;
                default:
                    break;
            }

        }

        




    }
}
