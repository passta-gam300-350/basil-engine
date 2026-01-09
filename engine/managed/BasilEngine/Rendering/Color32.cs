using Engine.Bindings;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BasilEngine.Rendering
{
    [Disabled]
    /// <summary>
    /// Represents an RGBA color with 8-bit integer components.
    /// </summary>
    public struct Color32
    {
        byte r, g, b, a;
        /// <summary>
        /// Initializes a new <see cref="Color32"/>.
        /// </summary>
        /// <param name="r">Red component (0-255).</param>
        /// <param name="g">Green component (0-255).</param>
        /// <param name="b">Blue component (0-255).</param>
        /// <param name="a">Alpha component (0-255).</param>
        public Color32(byte r, byte g, byte b, byte a = 255)
        {
            this.r = r;
            this.g = g;
            this.b = b;
            this.a = a;
        }
        /// <summary>
        /// Red component of the color.
        /// </summary>
        public byte R { get => r; set => r = value; }
        /// <summary>
        /// Green component of the color.
        /// </summary>
        public byte G { get => g; set => g = value; }
        /// <summary>
        /// Blue component of the color.
        /// </summary>
        public byte B { get => b; set => b = value; }
        /// <summary>
        /// Alpha component of the color.
        /// </summary>
        public byte A { get => a; set => a = value; }

        /// <summary>
        /// Solid white color (255,255,255,255).
        /// </summary>
        public static Color32 White => new Color32(255, 255, 255, 255);
        /// <summary>
        /// Solid black color (0,0,0,255).
        /// </summary>
        public static Color32 Black => new Color32(0, 0, 0, 255);
        /// <summary>
        /// Solid red color.
        /// </summary>
        public static Color32 Red => new Color32(255, 0, 0, 255);
        /// <summary>
        /// Solid green color.
        /// </summary>
        public static Color32 Green => new Color32(0, 255, 0, 255);
        /// <summary>
        /// Solid blue color.
        /// </summary>
        public static Color32 Blue => new Color32(0, 0, 255, 255);
        /// <summary>
        /// Solid magenta color.
        /// </summary>
        public static Color32 Magenta => new Color32(255, 0, 255, 255);
        /// <summary>
        /// Solid yellow color.
        /// </summary>
        public static Color32 Yellow => new Color32(255, 255, 0, 255);
        /// <summary>
        /// Gold color using 8-bit components.
        /// </summary>
        public static Color32 Gold => new Color32(255, 215, 0, 255);


        // Convertion from Color to Color32
        /// <summary>
        /// Converts a normalized <see cref="Color"/> to a <see cref="Color32"/>.
        /// </summary>
        /// <param name="color">Color with normalized channels.</param>
        public static implicit operator Color32(Color color)
        {
            return new Color32(
                (byte)(color.R * 255.0f),
                (byte)(color.G * 255.0f),
                (byte)(color.B * 255.0f),
                (byte)(color.A * 255.0f)
            );
        }

    }
}
