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

namespace BasilEngine.Mathematics
{

    public static class Mathf
    {
        public static float PI
        {
            get => 3.14159265358979323846f;
        }

        public static float Deg2Rad
        {
            get => PI / 180.0f;
        }

        public static float Rad2Deg
        {
            get => 180.0f / PI;
        }

        public static float Epsilon
        {
            get => float.Epsilon;
        }

        public static float Infinity
        {
            get => float.PositiveInfinity;
        }

        public static float NegativeInfinity
        {
            get => float.NegativeInfinity;
        }

        public static float Abs(float f)
        {
            return Math.Abs(f);
        }
        public static float Sin(float radians)
        {
            return (float)(Math.Sin((double)radians));
        }
        public static float Cos(float radians)
        {
            return (float)(Math.Cos((double)radians));
        }

        public static float DeltaAngle(float current, float target)
        {
            float num = Repeat(target - current, 360.0f);
            if (num > 180.0f)
            {
                num -= 360.0f;
            }
            return num;
        }
        public static float Acos(float f)
        {
            return (float)Math.Acos(f);
        }

        public static float Asin(float f)
        {
            return (float)Math.Asin(f);
        }

        public static float Atan(float f)
        {
            return (float)Math.Atan(f);
        }

        public static float Atan2(float y, float x)
        {
            return (float)Math.Atan2(y, x);
        }

        public static bool Approximately(float a, float b)
        {
            return Math.Abs(b - a) < Math.Max(1E-06f * Math.Max(Abs(a), Abs(b)), float.Epsilon * 8.0f);
        }

        public static float Ceil(float f)
        {
            return (float)Math.Ceiling(f);
        }

        public static int CeilToInt(float f)
        {
            return (int)Math.Ceiling(f);
        }
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

        public static int ClosestPowerOfTwo(int value)
        {
            return (int)Math.Pow(2.0, Math.Ceiling(Math.Log(value) / Math.Log(2.0)));
        }


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


        public static float Exp(float power)
        {
            return (float)Math.Exp(power);
        }

        public static ushort FloatToHalf(float f)
        {
            int num = BitConverter.ToInt32(BitConverter.GetBytes(f), 0);
            return (ushort)((num >> 16) & 65535);
        }

        public static float Floor(float f)
        {
            return (float)Math.Floor(f);
        }

        public static int FloorToInt(float f)
        {
            return (int)Math.Floor(f);
        }

        public static float GammaToLinearSpace(float f)
        {
            return (float)Math.Pow(f, 2.2);

        }

        public static float HalfToFloat(ushort val)
        {
            int num = (int)val << 16;
            return BitConverter.ToSingle(BitConverter.GetBytes(num), 0);
        }

        public static float InverseLerp(float a, float b, float value)
        {
            if (a != b)
            {
                return Clamp01((value - a) / (b - a));
            }
            return 0.0f;
        }

        public static bool IsPowerOfTwo(int value)
        {
            return value > 0 && (value & value - 1) == 0;
        }

        public static float Lerp(float a, float b, float t)
        {
            return a + (b - a) * Clamp01(t);
        }

        public static float LerpAngle(float a, float b, float t)
        {
            float num = Repeat(b - a, 360.0f);
            if (num > 180.0f)
            {
                num -= 360.0f;
            }
            return a + num * Clamp01(t);
        }

        public static float LerpUnclamped(float a, float b, float t)
        {
            return a + (b - a) * t;
        }

        public static float LinearToGammaSpace(float f)
        {
            return (float)Math.Pow(f, 1.0 / 2.2);
        }

        public static float Log(float f)
        {
            return (float)Math.Log(f);
        }

        public static float Log10(float f)
        {
            return (float)Math.Log10(f);
        }

        public static float Max(float a, float b)
        {
            return a > b ? a : b;
        }

        public static float Min(float a, float b)
        {
            return a < b ? a : b;
        }

        public static float MoveTowards(float current, float target, float maxDelta)
        {
            if (Abs(target - current) <= maxDelta)
            {
                return target;
            }
            return current + Sign(target - current) * maxDelta;
        }

        public static float MoveTowardsAngle(float current, float target, float maxDelta)
        {
            target = current + DeltaAngle(current, target);
            return MoveTowards(current, target, maxDelta);
        }

        public static float NextPowerOfTwo(int value)
        {
            return (float)Math.Pow(2.0, Math.Ceiling(Math.Log(value) / Math.Log(2.0)));
        }



        public static float PingPong(float t, float length)
        {
            t = Repeat(t, length * 2.0f);
            return length - Abs(t - length);
        }

        public static float Pow(float f, float p)
        {
            return (float)Math.Pow(f, p);
        }

        public static float Repeat(float t, float length)
        {
            return t - Floor(t / length) * length;
        }

        public static float Round(float f)
        {
            return (float)Math.Round(f);
        }

        public static int RoundToInt(float f)
        {
            return (int)Math.Round(f);
        }

        public static float Sign(float f)
        {
            return Math.Sign(f);
        }

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

        public static float SmoothDampAngle(float current, float target, ref float currentVelocity, float smoothTime, float maxSpeed, float deltaTime)
        {
            target = current + DeltaAngle(current, target);
            return SmoothDamp(current, target, ref currentVelocity, smoothTime, maxSpeed, deltaTime);
        }

        public static float SmoothStep(float from, float to, float t)
        {
            t = Clamp01(t);
            t = -2.0f * t * t * t + 3.0f * t * t;
            return to * t + from * (1.0f - t);
        }

        public static float Sqrt(float f)
        {
            return (float)Math.Sqrt(f);
        }

        public static float Tan(float f)
        {
            return (float)Math.Tan(f);
        }
    }
}
