using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Engine.Bindings;
namespace BasilEngine.Rendering
{
    // Color struct representing RGBA color normalized between 0 and 1
    [Disabled]
    public struct Color
    {
        
        float r,g, b, a;
        public Color(float r, float g, float b, float a = 1.0f)
        {
            this.r = r;
            this.g = g;
            this.b = b;
            this.a = a;
        }
        public float R { get => r; set => r = value; }
        public float G { get => g; set => g = value; }
        public float B { get => b; set => b = value; }
        public float A { get => a; set => a = value; }

        public static Color White => new Color(1.0f, 1.0f, 1.0f, 1.0f);

        public static Color Black => new Color(0.0f, 0.0f, 0.0f, 1.0f);

        public static Color Red => new Color(1.0f, 0.0f, 0.0f, 1.0f);
        public static Color Green => new Color(0.0f, 1.0f, 0.0f, 1.0f);
        public static Color Blue => new Color(0.0f, 0.0f, 1.0f, 1.0f);
        public static Color Magenta => new Color(1.0f, 0.0f, 1.0f, 1.0f);

        public static Color Yellow => new Color(1.0f, 1.0f, 0.0f, 1.0f);
        public static Color Gold => new Color(1.0f, 0.843f, 0.0f, 1.0f);

        // Convertion from Color32 to Color
        public static implicit operator Color(Color32 color32)
        {
            return new Color(
                color32.R / 255.0f,
                color32.G / 255.0f,
                color32.B / 255.0f,
                color32.A / 255.0f
            );
        }

    }
}
