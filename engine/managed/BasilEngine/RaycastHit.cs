using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using BasilEngine.Mathematics;
using Engine.Bindings;

namespace BasilEngine
{
    [Disabled]
    [StructLayout(LayoutKind.Sequential)]
    public struct RaycastHit
    {
        public GameObject entity;
        public Vector3 point;
        public Vector3 normal;
        public float distance;
        public byte hasHit;
        public byte isTrigger;

        public bool Hit => hasHit != 0;
        public bool Trigger => isTrigger != 0;
    }
    
}
