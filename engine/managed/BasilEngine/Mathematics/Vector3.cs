using Engine.Bindings;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace BasilEngine.Mathematics
{
    [Disabled]
    [StructLayout(LayoutKind.Sequential)]
    /// <summary>
    /// Represents a 3D vector and provides common vector operations.
    /// </summary>
    public struct Vector3
    {
        /// <summary>
        /// X component of the vector.
        /// </summary>
        public float x;
        /// <summary>
        /// Y component of the vector.
        /// </summary>
        public float y;
        /// <summary>
        /// Z component of the vector.
        /// </summary>
        public float z;
        /// <summary>
        /// Initializes a new <see cref="Vector3"/>.
        /// </summary>
        /// <param name="x">X component.</param>
        /// <param name="y">Y component.</param>
        /// <param name="z">Z component.</param>
        public Vector3(float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        /// <summary>
        /// Vector representing (0,0,0).
        /// </summary>
        public static Vector3 Zero => new Vector3(0, 0, 0);
        /// <summary>
        /// Up vector (0,1,0).
        /// </summary>
        public static Vector3 Up => new Vector3(0, 1, 0);
        /// <summary>
        /// Right vector (1,0,0).
        /// </summary>
        public static Vector3 Right => new Vector3(1, 0, 0);
        /// <summary>
        /// Forward vector (0,0,1).
        /// </summary>
        public static Vector3 Forward => new Vector3(0, 0, 1);

        public override string ToString()
        {
            return $"({x}, {y}, {z})";
        }

        // Operator overloads
        /// <summary>
        /// Adds two vectors component-wise.
        /// </summary>
        public static Vector3 operator +(Vector3 a, Vector3 b)
        {
            return new Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
        }

        /// <summary>
        /// Subtracts two vectors component-wise.
        /// </summary>
        public static Vector3 operator -(Vector3 a, Vector3 b)
        {
            return new Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
        }

        /// <summary>
        /// Multiplies a vector by a scalar.
        /// </summary>
        public static Vector3 operator *(Vector3 a, float scalar)
        {
            return new Vector3(a.x * scalar, a.y * scalar, a.z * scalar);
        }
        /// <summary>
        /// Divides a vector by a scalar.
        /// </summary>
        public static Vector3 operator /(Vector3 a, float scalar)
        {
            return new Vector3(a.x / scalar, a.y / scalar, a.z / scalar);
        }

        /// <summary>
        /// Multiplies a vector by a scalar.
        /// </summary>
        public static Vector3 operator *(float scalar, Vector3 a)
        {
            return new Vector3(a.x * scalar, a.y * scalar, a.z * scalar);
        }

        /// <summary>
        /// Negates a vector.
        /// </summary>
        public static Vector3 operator -(Vector3 a)
        {
            return new Vector3(-a.x, -a.y, -a.z);
        }
        
        /// <summary>
        /// Increments each component by 1.
        /// </summary>
        public static Vector3 operator ++(Vector3 a)
        {
            return new Vector3(a.x + 1, a.y + 1, a.z + 1);
        }
        /// <summary>
        /// Decrements each component by 1.
        /// </summary>
        public static Vector3 operator --(Vector3 a)
        {
            return new Vector3(a.x - 1, a.y - 1, a.z - 1);
        }

        // Dot product
        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        public static float Dot(Vector3 a, Vector3 b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        // Cross product
        /// <summary>
        /// Calculates the cross product of two vectors.
        /// </summary>
        public static Vector3 Cross(Vector3 a, Vector3 b)
        {
            return new Vector3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            );
        }
        // Magnitude
        /// <summary>
        /// Magnitude (length) of the vector.
        /// </summary>
        public float Magnitude()
        {
            return Mathf.Sqrt(x * x + y * y + z * z);
        }

        /// <summary>
        /// Squared magnitude of the vector.
        /// </summary>
        public float MagnitudeSqr()
        {
            return x * x + y * y + z * z;
        }
        // Normalize
        /// <summary>
        /// Returns a normalized copy of the vector.
        /// </summary>
        public Vector3 Normalize()
        {
            float mag = Magnitude();
            if (mag > 0)
            {
                return this / mag;
            }
            return Vector3.Zero;
        }
        // Distance
        /// <summary>
        /// Distance between two vectors.
        /// </summary>
        public static float Distance(Vector3 a, Vector3 b)
        {
            return (b - a).Magnitude();
        }
        /// <summary>
        /// Squared distance between two vectors.
        /// </summary>
        public static float DistanceSqr(Vector3 a, Vector3 b)
        {
            return (b - a).MagnitudeSqr();
        }
        // Lerp
        /// <summary>
        /// Linearly interpolates between two vectors with clamping.
        /// </summary>
        public static Vector3 Lerp(Vector3 a, Vector3 b, float t)
        {
            t = Mathf.Clamp(t, 0f, 1f);
            return new Vector3(
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t
            );
        }
    }
}
