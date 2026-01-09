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
    /// <summary>
    /// Represents the result of a physics raycast.
    /// </summary>
    public struct RaycastHit
    {
        /// <summary>
        /// Game object hit by the ray.
        /// </summary>
        public GameObject entity;
        /// <summary>
        /// Point of intersection in world space.
        /// </summary>
        public Vector3 point;
        /// <summary>
        /// Surface normal at the point of impact.
        /// </summary>
        public Vector3 normal;
        /// <summary>
        /// Distance from the ray origin to the hit point.
        /// </summary>
        public float distance;
        /// <summary>
        /// Non-zero if the ray registered a hit.
        /// </summary>
        public byte hasHit;
        /// <summary>
        /// Non-zero if the collider hit was a trigger.
        /// </summary>
        public byte isTrigger;

        /// <summary>
        /// Gets a value indicating whether the ray intersected any collider.
        /// </summary>
        public bool Hit => hasHit != 0;
        /// <summary>
        /// Gets a value indicating whether the hit collider was a trigger.
        /// </summary>
        public bool Trigger => isTrigger != 0;
    }
    
}
