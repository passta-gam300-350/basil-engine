using BasilEngine.Mathematics;
using Engine.Bindings;
using System.Runtime.InteropServices;

namespace BasilEngine
{
    [Disabled]
    [StructLayout(LayoutKind.Sequential)]
    public struct Ray
    {
        public Vector3 origin;
        public Vector3 direction;

        public Ray(Vector3 origin, Vector3 direction)
        {
            this.origin = origin;
            this.direction = direction;
        }
    }
}
