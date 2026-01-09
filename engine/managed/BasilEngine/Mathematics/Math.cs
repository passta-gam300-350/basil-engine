/* Start Header ************************************************************************/
/*!
\file		Math.cs
\title		Little Match
\author		Yeo Jia Hao, jiahao.yeo, 2301329
\par		jiahao.yeo\@digipen.edu
\date		24 March 2025
\brief		This file defines the Mathf class. It provides a set of mathematical functions and constants
            that are commonly used in game development. The functions include trigonometric,
            logarithmic, and interpolation functions, as well as color conversion functions.
            The class also includes methods for clamping values, calculating powers of two,
            and converting between float and half-precision formats.
*/
/* End Header **************************************************************************/

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using BasilEngine.Rendering;
using Engine.Bindings;

namespace BasilEngine.Mathematics
{
    [Disabled]
    /// <summary>
    /// Collection of math utilities mirroring common game engine APIs.
    /// </summary>
    public static class Mathf
    {
        /// <summary>
        /// The constant value of pi.
        /// </summary>
        public static float PI
        {
            get => 3.14159265358979323846f;
        }

        /// <summary>
        /// Conversion multiplier from degrees to radians.
        /// </summary>
        public static float Deg2Rad
        {
            get => PI / 180.0f;
        }

        /// <summary>
        /// Conversion multiplier from radians to degrees.
        /// </summary>
        public static float Rad2Deg
        {
            get => 180.0f / PI;
        }

        /// <summary>
        /// Smallest positive non-zero value greater than zero.
        /// </summary>
        public static float Epsilon
        {
            get => float.Epsilon;
        }

        /// <summary>
        /// Represents positive infinity.
        /// </summary>
        public static float Infinity
        {
            get => float.PositiveInfinity;
        }

        /// <summary>
        /// Represents negative infinity.
        /// </summary>
        public static float NegativeInfinity
        {
            get => float.NegativeInfinity;
        }

        /// <summary>
        /// Gets the absolute value of a float.
        /// </summary>
        /// <param name="f">Input value.</param>
        /// <returns>Absolute value of the input.</returns>
        public static float Abs(float f)
        {
            return Math.Abs(f);
        }
        /// <summary>
        /// Calculates the sine of an angle in radians.
        /// </summary>
        /// <param name="radians">Angle in radians.</param>
        /// <returns>Sine of the input angle.</returns>
        public static float Sin(float radians)
        {
            return (float)(Math.Sin((double)radians));
        }
        /// <summary>
        /// Calculates the cosine of an angle in radians.
        /// </summary>
        /// <param name="radians">Angle in radians.</param>
        /// <returns>Cosine of the input angle.</returns>
        public static float Cos(float radians)
        {
            return (float)(Math.Cos((double)radians));
        }

        /// <summary>
        /// Calculates the shortest delta between two angles in degrees.
        /// </summary>
        /// <param name="current">Current angle.</param>
        /// <param name="target">Target angle.</param>
        /// <returns>Signed minimal angle difference.</returns>
        public static float DeltaAngle(float current, float target)
        {
            float num = Repeat(target - current, 360.0f);
            if (num > 180.0f)
            {
                num -= 360.0f;
            }
            return num;
        }
        /// <summary>
        /// Calculates the arccosine of a value.
        /// </summary>
        /// <param name="f">Input value.</param>
        /// <returns>Angle in radians whose cosine is the input.</returns>
        public static float Acos(float f)
        {
            return (float)Math.Acos(f);
        }

        /// <summary>
        /// Calculates the arcsine of a value.
        /// </summary>
        /// <param name="f">Input value.</param>
        /// <returns>Angle in radians whose sine is the input.</returns>
        public static float Asin(float f)
        {
            return (float)Math.Asin(f);
        }

        /// <summary>
        /// Calculates the arctangent of a value.
        /// </summary>
        /// <param name="f">Input value.</param>
        /// <returns>Angle in radians whose tangent is the input.</returns>
        public static float Atan(float f)
        {
            return (float)Math.Atan(f);
        }

        /// <summary>
        /// Calculates the angle whose tangent is the quotient of two numbers.
        /// </summary>
        /// <param name="y">Y component.</param>
        /// <param name="x">X component.</param>
        /// <returns>Angle in radians between the positive X-axis and the point.</returns>
        public static float Atan2(float y, float x)
        {
            return (float)Math.Atan2(y, x);
        }

        /// <summary>
        /// Determines whether two floats are approximately equal.
        /// </summary>
        /// <param name="a">First value.</param>
        /// <param name="b">Second value.</param>
        /// <returns>True if the values are within a small epsilon.</returns>
        public static bool Approximately(float a, float b)
        {
            return Math.Abs(b - a) < Math.Max(1E-06f * Math.Max(Abs(a), Abs(b)), float.Epsilon * 8.0f);
        }

        /// <summary>
        /// Returns the smallest integral value greater than or equal to the specified number.
        /// </summary>
        /// <param name="f">Input value.</param>
        /// <returns>Ceiling of the input.</returns>
        public static float Ceil(float f)
        {
            return (float)Math.Ceiling(f);
        }

        /// <summary>
        /// Returns the smallest integral value greater than or equal to the specified number as an int.
        /// </summary>
        /// <param name="f">Input value.</param>
        /// <returns>Ceiling of the input as an int.</returns>
        public static int CeilToInt(float f)
        {
            return (int)Math.Ceiling(f);
        }
        /// <summary>
        /// Clamps a float between a minimum and maximum value.
        /// </summary>
        /// <param name="value">Input value.</param>
        /// <param name="min">Minimum allowed value.</param>
        /// <param name="max">Maximum allowed value.</param>
        /// <returns>Clamped value.</returns>
        public static float Clamp(float value, float min, float max)
        {
            if (value < min)
            {
                return min;
            }
            if (value > max)
            {
                return max;
            }
            return value;
        }

        /// <summary>
        /// Clamps an int between a minimum and maximum value.
        /// </summary>
        /// <param name="value">Input value.</param>
        /// <param name="min">Minimum allowed value.</param>
        /// <param name="max">Maximum allowed value.</param>
        /// <returns>Clamped value.</returns>
        public static int Clamp(int value, int min, int max)
        {
            if (value < min)
            {
                value = min;
            }
            else if (value > max)
            {
                value = max;
            }

            return value;
        }

        /// <summary>
        /// Clamps a value between 0 and 1.
        /// </summary>
        /// <param name="result">Input value.</param>
        /// <returns>Value clamped to the [0,1] range.</returns>
        public static float Clamp01(float result)
        {
            if (result < 0.0f)
            {
                return 0.0f;
            }
            if (result > 1.0f)
            {
                return 1.0f;
            }
            return result;
        }

        /// <summary>
        /// Finds the closest power of two that is greater than or equal to the specified value.
        /// </summary>
        /// <param name="value">Input value.</param>
        /// <returns>Next power of two.</returns>
        public static int ClosestPowerOfTwo(int value)
        {
            return (int)Math.Pow(2.0, Math.Ceiling(Math.Log(value) / Math.Log(2.0)));
        }


        /// <summary>
        /// Converts a correlated color temperature in Kelvin to an RGB color.
        /// </summary>
        /// <param name="kelvin">Temperature in Kelvin.</param>
        /// <returns>RGB color approximating the temperature.</returns>
        public static Color CorrelatedColorTemperatureToRGB(float kelvin)
        {


            float temp = kelvin / 100.0f;

            float red, green, blue;


            red = temp <= 66.0f ? 255.0f : 329.698727446f * (float)Math.Pow(temp - 60.0f, -0.1332047592f);
            green = temp <= 66.0f ? 99.4708025861f * (float)Math.Log(temp) - 161.1195681661f : 288.1221695283f * (float)Math.Pow(temp - 60.0f, -0.0755148492f);
            blue = temp >= 66.0f ? 255.0f : temp <= 19.0f ? 0.0f : 138.5177312231f * (float)Math.Log(temp - 10.0f) - 305.0447927307f;

            return new Color(Clamp01(red / 255.0f), Clamp01(green / 255.0f), Clamp01(blue / 255.0f));
            //return new Color(Clamp((int)red, 0, 255), Clamp((int)green, 0, 255), Clamp((int)blue, 0, 255));

        }


        /// <summary>
        /// Returns e raised to the specified power.
        /// </summary>
        /// <param name="power">Exponent to raise e to.</param>
        /// <returns>Result of e^power.</returns>
        public static float Exp(float power)
        {
            return (float)Math.Exp(power);
        }

        /// <summary>
        /// Converts a float into an unsigned 16-bit value representing the high 16 bits.
        /// </summary>
        /// <param name="f">Float to convert.</param>
        /// <returns>High 16 bits of the float.</returns>
        public static ushort FloatToHalf(float f)
        {
            int num = BitConverter.ToInt32(BitConverter.GetBytes(f), 0);
            return (ushort)((num >> 16) & 65535);
        }

        /// <summary>
        /// Returns the largest integer less than or equal to the specified value.
        /// </summary>
        /// <param name="f">Input value.</param>
        /// <returns>Floor of the input.</returns>
        public static float Floor(float f)
        {
            return (float)Math.Floor(f);
        }

        /// <summary>
        /// Returns the largest integer less than or equal to the specified value as an int.
        /// </summary>
        /// <param name="f">Input value.</param>
        /// <returns>Floor of the input as an int.</returns>
        public static int FloorToInt(float f)
        {
            return (int)Math.Floor(f);
        }

        /// <summary>
        /// Converts a value from gamma (sRGB) to linear space.
        /// </summary>
        /// <param name="f">Gamma-space value.</param>
        /// <returns>Linear-space value.</returns>
        public static float GammaToLinearSpace(float f)
        {
            return (float)Math.Pow(f, 2.2);

        }

        /// <summary>
        /// Converts the high 16 bits of an integer into a float.
        /// </summary>
        /// <param name="val">Half value.</param>
        /// <returns>Converted float.</returns>
        public static float HalfToFloat(ushort val)
        {
            int num = (int)val << 16;
            return BitConverter.ToSingle(BitConverter.GetBytes(num), 0);
        }

        /// <summary>
        /// Calculates the linear parameter that produces the interpolant value within the range [a, b].
        /// </summary>
        /// <param name="a">Range start.</param>
        /// <param name="b">Range end.</param>
        /// <param name="value">Value within the range.</param>
        /// <returns>Normalized position between 0 and 1.</returns>
        public static float InverseLerp(float a, float b, float value)
        {
            if (a != b)
            {
                return Clamp01((value - a) / (b - a));
            }
            return 0.0f;
        }

        /// <summary>
        /// Determines if a value is a power of two.
        /// </summary>
        /// <param name="value">Input value.</param>
        /// <returns>True if the value is a power of two.</returns>
        public static bool IsPowerOfTwo(int value)
        {
            return value > 0 && (value & value - 1) == 0;
        }

        /// <summary>
        /// Linearly interpolates between two values with clamping.
        /// </summary>
        /// <param name="a">Start value.</param>
        /// <param name="b">End value.</param>
        /// <param name="t">Interpolation factor in [0,1].</param>
        /// <returns>Interpolated value.</returns>
        public static float Lerp(float a, float b, float t)
        {
            return a + (b - a) * Clamp01(t);
        }

        /// <summary>
        /// Interpolates between two angles taking the shortest path.
        /// </summary>
        /// <param name="a">Start angle in degrees.</param>
        /// <param name="b">End angle in degrees.</param>
        /// <param name="t">Interpolation factor in [0,1].</param>
        /// <returns>Interpolated angle in degrees.</returns>
        public static float LerpAngle(float a, float b, float t)
        {
            float num = Repeat(b - a, 360.0f);
            if (num > 180.0f)
            {
                num -= 360.0f;
            }
            return a + num * Clamp01(t);
        }

        /// <summary>
        /// Linearly interpolates between two values without clamping the interpolation factor.
        /// </summary>
        /// <param name="a">Start value.</param>
        /// <param name="b">End value.</param>
        /// <param name="t">Interpolation factor.</param>
        /// <returns>Interpolated value.</returns>
        public static float LerpUnclamped(float a, float b, float t)
        {
            return a + (b - a) * t;
        }

        /// <summary>
        /// Converts a value from linear space to gamma (sRGB) space.
        /// </summary>
        /// <param name="f">Linear-space value.</param>
        /// <returns>Gamma-space value.</returns>
        public static float LinearToGammaSpace(float f)
        {
            return (float)Math.Pow(f, 1.0 / 2.2);
        }

        /// <summary>
        /// Natural logarithm of a specified value.
        /// </summary>
        /// <param name="f">Input value.</param>
        /// <returns>Natural log of the input.</returns>
        public static float Log(float f)
        {
            return (float)Math.Log(f);
        }

        /// <summary>
        /// Base-10 logarithm of a specified value.
        /// </summary>
        /// <param name="f">Input value.</param>
        /// <returns>Log base 10 of the input.</returns>
        public static float Log10(float f)
        {
            return (float)Math.Log10(f);
        }

        /// <summary>
        /// Returns the larger of two floating-point numbers.
        /// </summary>
        public static float Max(float a, float b)
        {
            return a > b ? a : b;
        }

        /// <summary>
        /// Returns the smaller of two floating-point numbers.
        /// </summary>
        public static float Min(float a, float b)
        {
            return a < b ? a : b;
        }

        /// <summary>
        /// Moves a value towards a target by a maximum change.
        /// </summary>
        /// <param name="current">Current value.</param>
        /// <param name="target">Target value.</param>
        /// <param name="maxDelta">Maximum change allowed.</param>
        /// <returns>New value after applying the move.</returns>
        public static float MoveTowards(float current, float target, float maxDelta)
        {
            if (Abs(target - current) <= maxDelta)
            {
                return target;
            }
            return current + Sign(target - current) * maxDelta;
        }

        /// <summary>
        /// Moves an angle towards a target angle by a maximum change, accounting for wraparound.
        /// </summary>
        public static float MoveTowardsAngle(float current, float target, float maxDelta)
        {
            target = current + DeltaAngle(current, target);
            return MoveTowards(current, target, maxDelta);
        }

        /// <summary>
        /// Returns the smallest power of two that is greater than or equal to the specified value.
        /// </summary>
        public static float NextPowerOfTwo(int value)
        {
            return (float)Math.Pow(2.0, Math.Ceiling(Math.Log(value) / Math.Log(2.0)));
        }



        /// <summary>
        /// Ping-pongs a value between 0 and length.
        /// </summary>
        public static float PingPong(float t, float length)
        {
            t = Repeat(t, length * 2.0f);
            return length - Abs(t - length);
        }

        /// <summary>
        /// Raises a number to a specified power.
        /// </summary>
        public static float Pow(float f, float p)
        {
            return (float)Math.Pow(f, p);
        }

        /// <summary>
        /// Repeats a value in the range [0, length).
        /// </summary>
        public static float Repeat(float t, float length)
        {
            return t - Floor(t / length) * length;
        }

        /// <summary>
        /// Rounds a value to the nearest integer.
        /// </summary>
        public static float Round(float f)
        {
            return (float)Math.Round(f);
        }

        /// <summary>
        /// Rounds a value to the nearest integer and returns it as an int.
        /// </summary>
        public static int RoundToInt(float f)
        {
            return (int)Math.Round(f);
        }

        /// <summary>
        /// Gets the sign of a value.
        /// </summary>
        public static float Sign(float f)
        {
            return Math.Sign(f);
        }

        /// <summary>
        /// Smoothly damps a value towards a target using a critically damped spring function.
        /// </summary>
        public static float SmoothDamp(float current, float target, ref float currentVelocity, float smoothTime, float maxSpeed, float deltaTime)
        {
            smoothTime = Max(0.0001f, smoothTime);
            float num1 = 2.0f / smoothTime;
            float num2 = num1 * deltaTime;
            float num3 = (float)(1.0f /
                          (1.0f + num2 + 0.479999989271164 * num2 * num2 + 0.234999999403954 * num2 * num2 * num2));
            float num4 = current - target;
            float num5 = target;
            float num6 = maxSpeed * smoothTime;
            num4 = Clamp(num4, -num6, num6);
            target = current - num4;
            float num7 = (currentVelocity + num1 * num4) * deltaTime;
            currentVelocity = (currentVelocity - num1 * num7) * num3;
            float num8 = target + (num4 + num7) * num3;
            if ((double)num5 - (double)current > 0.0 == (double)num8 > (double)num5)
            {
                num8 = num5;
                currentVelocity = (num8 - num5) / deltaTime;
            }
            return num8;
        }

        /// <summary>
        /// Smoothly damps an angle towards a target angle using a critically damped spring function.
        /// </summary>
        public static float SmoothDampAngle(float current, float target, ref float currentVelocity, float smoothTime, float maxSpeed, float deltaTime)
        {
            target = current + DeltaAngle(current, target);
            return SmoothDamp(current, target, ref currentVelocity, smoothTime, maxSpeed, deltaTime);
        }

        /// <summary>
        /// Interpolates between two values using a smooth cubic curve.
        /// </summary>
        public static float SmoothStep(float from, float to, float t)
        {
            t = Clamp01(t);
            t = -2.0f * t * t * t + 3.0f * t * t;
            return to * t + from * (1.0f - t);
        }

        /// <summary>
        /// Returns the square root of a value.
        /// </summary>
        public static float Sqrt(float f)
        {
            return (float)Math.Sqrt(f);
        }

        /// <summary>
        /// Returns the tangent of the specified angle in radians.
        /// </summary>
        public static float Tan(float f)
        {
            return (float)Math.Tan(f);
        }
    }
}
