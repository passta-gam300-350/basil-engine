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
    /// <summary>
    /// A Vector2 struct that provides 2D vector operations.
    /// </summary>
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct Vector2
    {
        public float x;
        public float y;

        //Vector2()
        //{
        //    x = 0;
        //    y = 0;
        //}
        /// <summary>
        /// Get the distance between two vectors.
        /// </summary>
        /// <param name="a">The lhs vector</param>
        /// <param name="b">The rhs vector</param>
        /// <returns>The distane between the two vector.</returns>
        public static float Distance(Vector2 a, Vector2 b)
        {
            return new Vector2(b.x - a.x, b.y - a.y).magnitude;
        }

        /// <summary>
        /// Convert the vector to a string.
        /// </summary>
        /// <returns>A string representaiton of the vector.</returns>
        public override string ToString()
        {
            return $"({x}, {y})";
        }
        /// <summary>
        /// The magnitude of the vector.
        /// </summary>
        public float magnitude
        {
            get => Magnitude();
        }
        /// <summary>
        /// The magnitude of the vector squared.
        /// </summary>
        public float magnitudeSqr
        {
            get => MagnitudeSqr();
        }
        /// <summary>
        /// Create a vector with x and y.
        /// </summary>
        /// <param name="x">The x value</param>
        /// <param name="y">The y value</param>
        public Vector2(float x, float y)
        {
            this.x = x;
            this.y = y;
        }

        public static Vector2 operator +(Vector2 a, Vector2 b)
        {
            return new Vector2(a.x + b.x, a.y + b.y);
        }

        public static Vector2 operator -(Vector2 a, Vector2 b)
        {
            return new Vector2(a.x - b.x, a.y - b.y);
        }

        public static Vector2 operator *(Vector2 a, float b)
        {
            return new Vector2(a.x * b, a.y * b);
        }

        public static Vector2 operator *(float a, Vector2 b)
        {
            return new Vector2(a * b.x, a * b.y);
        }


        public static Vector2 operator /(Vector2 a, float b)
        {
            return new Vector2(a.x / b, a.y / b);
        }

        public static float operator *(Vector2 a, Vector2 b)
        {
            return Dot(a, b);
        }
        public static float Dot(Vector2 a, Vector2 b)
        {
            return a.x * b.x + a.y * b.y;
        }
        /// <summary>
        /// Cross product of two vectors.
        /// </summary>
        /// <param name="a">LHS vector</param>
        /// <param name="b">RHS vector</param>
        /// <returns>A float value for 2d vector</returns>
        public static float Cross(Vector2 a, Vector2 b)
        {
            return a.x * b.y - a.y * b.x;
        }
        /// <summary>
        /// Magnitude of the vector.
        /// </summary>
        /// <returns>The magnitude</returns>
        public float Magnitude()
        {
            return (float)System.Math.Sqrt(x * x + y * y);
        }
        /// <summary>
        /// Get the magnitude squared.
        /// </summary>
        /// <returns>Magnitude squared of the vector</returns>
        public float MagnitudeSqr()
        {
            return x * x + y * y;
        }
        /// <summary>
        /// Normalize the vector.
        /// </summary>
        /// <returns>Get the normalized vector</returns>
        public Vector2 Normalize()
        {
            return this / Magnitude();
        }




        public static Vector2 Lerp(Vector2 a, Vector2 b, float t)
        {
            return a + (b - a) * t;
        }


        public static Vector2 SmoothDamp(Vector2 current, Vector2 target, ref Vector2 currentVelocity, float smoothTime, float maxSpeed, float deltaTime)
        {
            float num = 0f;
            float num2 = 0f;
            float num3 = 0f;
            smoothTime = Mathf.Max(0.0001f, smoothTime);
            float num4 = 2f / smoothTime;
            float num5 = num4 * deltaTime;
            float num6 = 1f / (1f + num5 + 0.48f * num5 * num5 + 0.235f * num5 * num5 * num5);
            float num7 = current.x - target.x;
            float num8 = current.y - target.y;
            // float num9 = current.z - target.z;
            Vector2 vector = target;
            float num10 = maxSpeed * smoothTime;
            float num11 = num10 * num10;
            float num12 = num7 * num7 + num8 * num8;
            if (num12 > num11)
            {
                float num13 = (float)Math.Sqrt(num12);
                num7 = num7 / num13 * num10;
                num8 = num8 / num13 * num10;
            }

            target.x = current.x - num7;
            target.y = current.y - num8;

            float num14 = (currentVelocity.x + num4 * num7) * deltaTime;
            float num15 = (currentVelocity.y + num4 * num8) * deltaTime;

            currentVelocity.x = (currentVelocity.x - num4 * num14) * num6;
            currentVelocity.y = (currentVelocity.y - num4 * num15) * num6;

            num = target.x + (num7 + num14) * num6;
            num2 = target.y + (num8 + num15) * num6;

            float num17 = vector.x - current.x;
            float num18 = vector.y - current.y;

            float num20 = num - vector.x;
            float num21 = num2 - vector.y;

            if (num17 * num20 + num18 * num21 > 0f)
            {
                num = vector.x;
                num2 = vector.y;

                currentVelocity.x = (num - vector.x) / deltaTime;
                currentVelocity.y = (num2 - vector.y) / deltaTime;
            }

            return new Vector2(num, num2);
        }

    }
}
