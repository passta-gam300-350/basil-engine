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
                public Vector3 Point;
        public Vector3 Normal;
        public float Distance;
        
    }
    
}
