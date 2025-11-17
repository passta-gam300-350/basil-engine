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
    public class Transform : Component
    {

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetPosition")]
        [StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]

        public extern static void SetPosition(UInt64 handle, float x, float y, float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetPosition")]
        [StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]

        public static extern void GetPosition(UInt64 handle, out float x, out float y, out float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetRotation")]
        [StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]
        public extern static void SetRotationInternal(UInt64 handle, float x, float y, float z);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetRotation")]
        [StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]
        public extern static void GetRotationInternal(UInt64 handle, out float x, out float y, out float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetScale")]
        [StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]
        public extern static void SetScaleInternal(UInt64 handle, float x, float y, float z);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetScale")]
		[StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]
		public extern static void GetScaleInternal(UInt64 handle, out float x, out float y, out float z);

		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeMethod("GetForward")]
		[StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]
		public extern static void GetForwardInternal(UInt64 handle, out float x, out float y, out float z);

		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeMethod("GetRight")]
		[StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]
		public extern static void GetRightInternal(UInt64 handle, out float x, out float y, out float z);

		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeMethod("GetUp")]
		[StaticAccessor("ManagedTransform", StaticAccessorType.DoubleColon)]
		public extern static void GetUpInternal(UInt64 handle, out float x, out float y, out float z);



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


        public Transform(UInt64 handle)
        {
            NativeID = handle; // GameObject Handle
        }


		public Vector3 forward
		{
			get
			{
				GetForwardInternal(NativeID, out float x, out float y, out float z);
				return new Vector3(x, y, z);
			}
		}

		public Vector3 right
		{
			get
			{
				GetRightInternal(NativeID, out float x, out float y, out float z);
				return new Vector3(x, y, z);
			}
		}

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
