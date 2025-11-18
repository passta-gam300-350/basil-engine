#version 450 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D u_Texture;

// Convert linear RGB to sRGB (manual gamma correction)
vec3 linearToSRGB(vec3 linear) {
    // Standard sRGB gamma curve (accurate piecewise function)
    vec3 srgb;
    for (int i = 0; i < 3; ++i) {
        if (linear[i] <= 0.0031308) {
            srgb[i] = 12.92 * linear[i];
        } else {
            srgb[i] = 1.055 * pow(linear[i], 1.0/2.4) - 0.055;
        }
    }
    return srgb;
}

void main()
{
    // Sample from source texture (could be RGB16F or SRGB8)
    // If SRGB8: GPU auto-converts sRGB→linear
    // If RGB16F: Already linear
    vec3 linearColor = texture(u_Texture, TexCoords).rgb;

    // Manually apply gamma correction (linear → sRGB encoding)
    // This ensures correct display without GL_FRAMEBUFFER_SRGB
    vec3 srgbColor = linearToSRGB(linearColor);

    // Output gamma-corrected values to RGB8 buffer
    // ImGui will display these without conversion = correct brightness!
    FragColor = vec4(srgbColor, 1.0);
}
