using BasilEngine.Mathematics;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.CompilerServices;
namespace BasilEngine.Components
{
    public class Transform : Component
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern void SetPosition(in Vector3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern void GetPosition(out Vector3 pos);

        public Vector3 position
        {
            get
            {
                GetPosition(out Vector3 pos);
                return pos;
            }
            set
            {
                SetPosition(in value);
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

        public Transform(UInt32 handle)
        {
            NativeID = handle; // GameObject Handle
        }

    }
}
