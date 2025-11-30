using BasilEngine.Mathematics;
using Engine.Bindings;
using System.Runtime.InteropServices;

namespace BasilEngine
{
    [Disabled]
    [StructLayout(LayoutKind.Sequential)]
    /// <summary>
    /// Represents a ray with an origin and direction.
    /// </summary>
    public struct Ray
    {
        /// <summary>
        /// Starting point of the ray in world space.
        /// </summary>
        public Vector3 origin;
        /// <summary>
        /// Direction vector of the ray (should be normalized).
        /// </summary>
        public Vector3 direction;

        /// <summary>
        /// Initializes a new <see cref="Ray"/>.
        /// </summary>
        /// <param name="origin">Ray origin.</param>
        /// <param name="direction">Ray direction.</param>
        public Ray(Vector3 origin, Vector3 direction)
        {
            this.origin = origin;
            this.direction = direction;
        }
    }
}
