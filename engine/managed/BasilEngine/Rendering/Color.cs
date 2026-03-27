using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Engine.Bindings;
using BasilEngine.Mathematics;
namespace BasilEngine.Rendering
{

    // Color struct representing RGBA color normalized between 0 and 1
    [Disabled]
    /// <summary>
    /// Represents an RGBA color with components normalized between 0 and 1.
    /// </summary>
    public struct Color
    {
        
        float r,g, b, a;
        /// <summary>
        /// Initializes a new <see cref="Color"/>.
        /// </summary>
        /// <param name="r">Red component (0-1).</param>
        /// <param name="g">Green component (0-1).</param>
        /// <param name="b">Blue component (0-1).</param>
        /// <param name="a">Alpha component (0-1).</param>
        public Color(float r, float g, float b, float a = 1.0f)
        {
            this.r = r;
            this.g = g;
            this.b = b;
            this.a = a;
        }
        /// <summary>
        /// Red component of the color.
        /// </summary>
        public float R { get => r; set => r = value; }
        /// <summary>
        /// Green component of the color.
        /// </summary>
        public float G { get => g; set => g = value; }
        /// <summary>
        /// Blue component of the color.
        /// </summary>
        public float B { get => b; set => b = value; }
        /// <summary>
        /// Alpha component of the color.
        /// </summary>
        public float A { get => a; set => a = value; }

        /// <summary>
        /// Solid white color (1,1,1,1).
        /// </summary>
        public static Color White => new Color(1.0f, 1.0f, 1.0f, 1.0f);

        /// <summary>
        /// Solid black color (0,0,0,1).
        /// </summary>
        public static Color Black => new Color(0.0f, 0.0f, 0.0f, 1.0f);

        /// <summary>
        /// Solid red color.
        /// </summary>
        public static Color Red => new Color(1.0f, 0.0f, 0.0f, 1.0f);
        /// <summary>
        /// Solid green color.
        /// </summary>
        public static Color Green => new Color(0.0f, 1.0f, 0.0f, 1.0f);
        /// <summary>
        /// Solid blue color.
        /// </summary>
        public static Color Blue => new Color(0.0f, 0.0f, 1.0f, 1.0f);
        /// <summary>
        /// Solid magenta color.
        /// </summary>
        public static Color Magenta => new Color(1.0f, 0.0f, 1.0f, 1.0f);

        /// <summary>
        /// Solid yellow color.
        /// </summary>
        public static Color Yellow => new Color(1.0f, 1.0f, 0.0f, 1.0f);
        /// <summary>
        /// Gold color using normalized RGBA values.
        /// </summary>
        public static Color Gold => new Color(1.0f, 0.843f, 0.0f, 1.0f);

        // Convertion from Color32 to Color
        /// <summary>
        /// Converts a <see cref="Color32"/> to a normalized <see cref="Color"/>.
        /// </summary>
        /// <param name="color32">Color with 8-bit components.</param>
        public static implicit operator Color(Color32 color32)
        {
            return new Color(
                color32.R / 255.0f,
                color32.G / 255.0f,
                color32.B / 255.0f,
                color32.A / 255.0f
            );
        }

        /// <summary>
        /// Converts a <see cref="Vector4"/> to a <see cref="Color"/>.
        /// </summary>
        /// <param name="vector">Vector containing RGBA values.</param>
        public static implicit operator Color(Vector4 vector)
        {
            return new Color(vector.x, vector.y, vector.z, vector.w);
        }

        /// <summary>
        /// Converts a <see cref="Color"/> to a <see cref="Vector4"/>.
        /// </summary>
        /// <param name="color">Color containing RGBA values.</param>
        public static implicit operator Vector4(Color color)
        {
            return new Vector4(color.R, color.G, color.B, color.A);
        }

        

    }
}
