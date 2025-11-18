using Engine.Bindings;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BasilEngine.Mathematics
{
    [Disabled]
    public struct Vector3
    {
        public float x;
        public float y;
        public float z;
        public Vector3(float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        public static Vector3 Zero => new Vector3(0, 0, 0);
        public static Vector3 Up => new Vector3(0, 1, 0);
        public static Vector3 Right => new Vector3(1, 0, 0);
        public static Vector3 Forward => new Vector3(0, 0, 1);

        public override string ToString()
        {
            return $"({x}, {y}, {z})";
        }

        // Operator overloads
        public static Vector3 operator +(Vector3 a, Vector3 b)
        {
            return new Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
        }

        public static Vector3 operator -(Vector3 a, Vector3 b)
        {
            return new Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
        }

        public static Vector3 operator *(Vector3 a, float scalar)
        {
            return new Vector3(a.x * scalar, a.y * scalar, a.z * scalar);
        }
        public static Vector3 operator /(Vector3 a, float scalar)
        {
            return new Vector3(a.x / scalar, a.y / scalar, a.z / scalar);
        }

        public static Vector3 operator *(float scalar, Vector3 a)
        {
            return new Vector3(a.x * scalar, a.y * scalar, a.z * scalar);
        }

        public static Vector3 operator -(Vector3 a)
        {
            return new Vector3(-a.x, -a.y, -a.z);
        }
        
        public static Vector3 operator ++(Vector3 a)
        {
            return new Vector3(a.x + 1, a.y + 1, a.z + 1);
        }
        public static Vector3 operator --(Vector3 a)
        {
            return new Vector3(a.x - 1, a.y - 1, a.z - 1);
        }

        // Dot product
        public static float Dot(Vector3 a, Vector3 b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        // Cross product
        public static Vector3 Cross(Vector3 a, Vector3 b)
        {
            return new Vector3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            );
        }
        // Magnitude
        public float Magnitude()
        {
            return Mathf.Sqrt(x * x + y * y + z * z);
        }

        public float MagnitudeSqr()
        {
            return x * x + y * y + z * z;
        }
        // Normalize
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
        public static float Distance(Vector3 a, Vector3 b)
        {
            return (b - a).Magnitude();
        }
        public static float DistanceSqr(Vector3 a, Vector3 b)
        {
            return (b - a).MagnitudeSqr();
        }
        // Lerp
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
