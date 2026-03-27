using Engine.Bindings;
using System.Runtime.InteropServices;

namespace BasilEngine.Mathematics
{
    [Disabled]
    [StructLayout(LayoutKind.Sequential)]
    /// <summary>
    /// Represents a 4D vector and provides common vector operations.
    /// </summary>
    public struct Vector4
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
        /// W component of the vector.
        /// </summary>
        public float w;

        /// <summary>
        /// Initializes a new <see cref="Vector4"/>.
        /// </summary>
        public Vector4(float x, float y, float z, float w)
        {
            this.x = x;
            this.y = y;
            this.z = z;
            this.w = w;
        }

        /// <summary>
        /// Vector representing (0, 0, 0, 0).
        /// </summary>
        public static Vector4 Zero => new Vector4(0f, 0f, 0f, 0f);

        /// <summary>
        /// Vector representing (1, 1, 1, 1).
        /// </summary>
        public static Vector4 One => new Vector4(1f, 1f, 1f, 1f);

        public override string ToString()
        {
            return $"({x}, {y}, {z}, {w})";
        }

        /// <summary>
        /// Adds two vectors component-wise.
        /// </summary>
        public static Vector4 operator +(Vector4 a, Vector4 b)
        {
            return new Vector4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
        }

        /// <summary>
        /// Subtracts two vectors component-wise.
        /// </summary>
        public static Vector4 operator -(Vector4 a, Vector4 b)
        {
            return new Vector4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
        }

        /// <summary>
        /// Multiplies a vector by a scalar.
        /// </summary>
        public static Vector4 operator *(Vector4 a, float scalar)
        {
            return new Vector4(a.x * scalar, a.y * scalar, a.z * scalar, a.w * scalar);
        }

        /// <summary>
        /// Multiplies a vector by a scalar.
        /// </summary>
        public static Vector4 operator *(float scalar, Vector4 a)
        {
            return new Vector4(a.x * scalar, a.y * scalar, a.z * scalar, a.w * scalar);
        }

        /// <summary>
        /// Divides a vector by a scalar.
        /// </summary>
        public static Vector4 operator /(Vector4 a, float scalar)
        {
            return new Vector4(a.x / scalar, a.y / scalar, a.z / scalar, a.w / scalar);
        }

        /// <summary>
        /// Negates a vector.
        /// </summary>
        public static Vector4 operator -(Vector4 a)
        {
            return new Vector4(-a.x, -a.y, -a.z, -a.w);
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        public static float Dot(Vector4 a, Vector4 b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        }

        /// <summary>
        /// Magnitude (length) of the vector.
        /// </summary>
        public float Magnitude()
        {
            return Mathf.Sqrt(x * x + y * y + z * z + w * w);
        }

        /// <summary>
        /// Squared magnitude of the vector.
        /// </summary>
        public float MagnitudeSqr()
        {
            return x * x + y * y + z * z + w * w;
        }

        /// <summary>
        /// Returns a normalized copy of the vector.
        /// </summary>
        public Vector4 Normalize()
        {
            float mag = Magnitude();
            if (mag > 0f)
            {
                return this / mag;
            }

            return Zero;
        }

        /// <summary>
        /// Distance between two vectors.
        /// </summary>
        public static float Distance(Vector4 a, Vector4 b)
        {
            return (b - a).Magnitude();
        }

        /// <summary>
        /// Squared distance between two vectors.
        /// </summary>
        public static float DistanceSqr(Vector4 a, Vector4 b)
        {
            return (b - a).MagnitudeSqr();
        }

        /// <summary>
        /// Linearly interpolates between two vectors with clamping.
        /// </summary>
        public static Vector4 Lerp(Vector4 a, Vector4 b, float t)
        {
            t = Mathf.Clamp(t, 0f, 1f);
            return new Vector4(
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t,
                a.w + (b.w - a.w) * t
            );
        }
    }
}
