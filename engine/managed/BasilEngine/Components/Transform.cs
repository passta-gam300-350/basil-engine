using BasilEngine.Mathematics;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.CompilerServices;
using Engine.Bindings;
namespace BasilEngine.Components
{



    [NativeHeader("Bindings/ManagedTransform.hpp")]
    [NativeClass("ManagedTransform")]
    /// <summary>
    /// Provides position, rotation, and scale accessors for a scene object.
    /// </summary>
    public class Transform : Component
    {

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetPosition")]
        [StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Writes a world-space position to the native transform.
        /// </summary>
        /// <param name="handle">Native game object handle.</param>
        /// <param name="x">X coordinate.</param>
        /// <param name="y">Y coordinate.</param>
        /// <param name="z">Z coordinate.</param>
        public extern static void SetPosition(UInt64 handle, float x, float y, float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetPosition")]
        [StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Reads the world-space position from the native transform.
        /// </summary>
        /// <param name="handle">Native game object handle.</param>
        /// <param name="x">Receives the X coordinate.</param>
        /// <param name="y">Receives the Y coordinate.</param>
        /// <param name="z">Receives the Z coordinate.</param>
        public static extern void GetPosition(UInt64 handle, out float x, out float y, out float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetRotation")]
        [StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Writes Euler angles to the native transform.
        /// </summary>
        /// <param name="handle">Native game object handle.</param>
        /// <param name="x">Pitch component in degrees.</param>
        /// <param name="y">Yaw component in degrees.</param>
        /// <param name="z">Roll component in degrees.</param>
        public extern static void SetRotationInternal(UInt64 handle, float x, float y, float z);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetRotation")]
        [StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Reads Euler angles from the native transform.
        /// </summary>
        /// <param name="handle">Native game object handle.</param>
        /// <param name="x">Pitch component in degrees.</param>
        /// <param name="y">Yaw component in degrees.</param>
        /// <param name="z">Roll component in degrees.</param>
        public extern static void GetRotationInternal(UInt64 handle, out float x, out float y, out float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetScale")]
        [StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Writes local scale to the native transform.
        /// </summary>
        /// <param name="handle">Native game object handle.</param>
        /// <param name="x">Scale on X.</param>
        /// <param name="y">Scale on Y.</param>
        /// <param name="z">Scale on Z.</param>
        public extern static void SetScaleInternal(UInt64 handle, float x, float y, float z);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetScale")]
		[StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]
		/// <summary>
		/// Reads local scale from the native transform.
		/// </summary>
		/// <param name="handle">Native game object handle.</param>
		/// <param name="x">Scale on X.</param>
		/// <param name="y">Scale on Y.</param>
		/// <param name="z">Scale on Z.</param>
		public extern static void GetScaleInternal(UInt64 handle, out float x, out float y, out float z);

		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeMethod("GetForward")]
		[StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]
		/// <summary>
		/// Retrieves the forward direction vector from the native transform.
		/// </summary>
		/// <param name="handle">Native game object handle.</param>
		/// <param name="x">X component of the forward vector.</param>
		/// <param name="y">Y component of the forward vector.</param>
		/// <param name="z">Z component of the forward vector.</param>
		public extern static void GetForwardInternal(UInt64 handle, out float x, out float y, out float z);

		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeMethod("GetRight")]
		[StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]
		/// <summary>
		/// Retrieves the right direction vector from the native transform.
		/// </summary>
		/// <param name="handle">Native game object handle.</param>
		/// <param name="x">X component of the right vector.</param>
		/// <param name="y">Y component of the right vector.</param>
		/// <param name="z">Z component of the right vector.</param>
		public extern static void GetRightInternal(UInt64 handle, out float x, out float y, out float z);

		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeMethod("GetUp")]
		[StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]
		/// <summary>
		/// Retrieves the up direction vector from the native transform.
		/// </summary>
		/// <param name="handle">Native game object handle.</param>
		/// <param name="x">X component of the up vector.</param>
		/// <param name="y">Y component of the up vector.</param>
		/// <param name="z">Z component of the up vector.</param>
		public extern static void GetUpInternal(UInt64 handle, out float x, out float y, out float z);



        /// <summary>
        /// World position of the transform.
        /// </summary>
        public Vector3 position
        {
            get
            {
                GetPosition(NativeID, out float x, out float y, out float z);
                return new Vector3(x, y, z);
            }
            set
            {
                SetPosition(NativeID, value.x, value.y, value.z);
            }
        }
        /// <summary>
        /// Euler rotation of the transform in degrees.
        /// </summary>
        public Vector3 rotation
        {
            get
            {
                GetRotationInternal(NativeID, out float x, out float y, out float z);
                return new Vector3(x, y, z);
            }
            set
            {
                SetRotationInternal(NativeID, value.x, value.y, value.z);
            }
        }

        /// <summary>
        /// Local scale of the transform.
        /// </summary>
        public Vector3 scale
        {
            get
            {
                GetScaleInternal(NativeID, out float x, out float y, out float z);
                return new Vector3(x, y, z);
            }
            set
            {
                SetScaleInternal(NativeID, value.x, value.y, value.z);
            }
        }


        /// <summary>
        /// Creates a managed transform bound to an existing native object.
        /// </summary>
        /// <param name="handle">Native game object handle.</param>
        public Transform(UInt64 handle)
        {
            NativeID = handle; // GameObject Handle
        }


		/// <summary>
		/// Forward direction of the transform in world space.
		/// </summary>
		public Vector3 forward
		{
			get
			{
				GetForwardInternal(NativeID, out float x, out float y, out float z);
				return new Vector3(x, y, z);
			}
		}

		/// <summary>
		/// Right direction of the transform in world space.
		/// </summary>
		public Vector3 right
		{
			get
			{
				GetRightInternal(NativeID, out float x, out float y, out float z);
				return new Vector3(x, y, z);
			}
		}

		/// <summary>
		/// Up direction of the transform in world space.
		/// </summary>
		public Vector3 up
		{
			get
			{
				GetUpInternal(NativeID, out float x, out float y, out float z);
				return new Vector3(x, y, z);
			}
		}

    }
}
