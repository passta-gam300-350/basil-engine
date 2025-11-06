using Engine.Bindings;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BasilEngine.Rendering
{
    [Disabled]
    public struct Color32
    {
        byte r, g, b, a;
        public Color32(byte r, byte g, byte b, byte a = 255)
        {
            this.r = r;
            this.g = g;
            this.b = b;
            this.a = a;
        }
        public byte R { get => r; set => r = value; }
        public byte G { get => g; set => g = value; }
        public byte B { get => b; set => b = value; }
        public byte A { get => a; set => a = value; }

        public static Color32 White => new Color32(255, 255, 255, 255);
        public static Color32 Black => new Color32(0, 0, 0, 255);
        public static Color32 Red => new Color32(255, 0, 0, 255);
        public static Color32 Green => new Color32(0, 255, 0, 255);
        public static Color32 Blue => new Color32(0, 0, 255, 255);
        public static Color32 Magenta => new Color32(255, 0, 255, 255);
        public static Color32 Yellow => new Color32(255, 255, 0, 255);
        public static Color32 Gold => new Color32(255, 215, 0, 255);


        // Convertion from Color to Color32
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
