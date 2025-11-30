using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.CompilerServices;
using Engine.Bindings;
using BasilEngine.Mathematics;

namespace BasilEngine
{
    [NativeHeader("Bindings/ManagedPhysics.hpp")]
    [NativeClass("ManagedPhysics")]
    /// <summary>
    /// Provides access to physics queries exposed by the native engine.
    /// </summary>
    public class Physics
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("Raycast")]
        [StaticAccessor("ManagedPhysics", StaticAccessorType.DoubleColon)]
        private static extern bool Internal_Raycast(float ox, float oy, float oz,
                                                    float dx, float dy, float dz,
                                                    float maxDistance, bool ignoreTriggers,
                                                    out ulong entity,
                                                    out float px, out float py, out float pz,
                                                    out float nx, out float ny, out float nz,
                                                    out float distance,
                                                    out byte isTrigger);

        /// <summary>
        /// Casts a ray in the physics world.
        /// </summary>
        /// <param name="origin">World-space origin of the ray.</param>
        /// <param name="direction">Normalized direction to cast along.</param>
        /// <param name="hit">Information about the hit if one occurred.</param>
        /// <param name="maxDistance">Maximum distance the ray should travel.</param>
        /// <param name="ignoreTriggers">True to ignore trigger colliders.</param>
        /// <returns>True if the ray intersected a collider.</returns>
        public static bool Raycast(Vector3 origin, Vector3 direction, out RaycastHit hit, float maxDistance = 1000f, bool ignoreTriggers = true)
        {
            var hasHit = Internal_Raycast(origin.x, origin.y, origin.z,
                                          direction.x, direction.y, direction.z,
                                          maxDistance, ignoreTriggers,
                                          out var entity,
                                          out var px, out var py, out var pz,
                                          out var nx, out var ny, out var nz,
                                          out var distance,
                                          out var isTrigger);

            hit = new RaycastHit
            {
                entity = new GameObject(entity),
                point = new Vector3(px, py, pz),
                normal = new Vector3(nx, ny, nz),
                distance = distance,
                hasHit = (byte)(hasHit ? 1 : 0),
                isTrigger = isTrigger
            };
            return hasHit;
        }
    }
}
