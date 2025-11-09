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

        public static  extern void GetPosition(UInt64 handle, out float x, out float y, out float z);

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
            get;
            set;
        } = Vector3.Zero;
        public Vector3 scale
        {
            get;
            set;
        } = new Vector3(1, 1, 1);

        public Transform(UInt64 handle)
        {
            NativeID = handle; // GameObject Handle
        }

    }
}
