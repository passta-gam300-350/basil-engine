#version 450 core

in vec2 TexCoords;
out vec4 FragColor;

// ===== Uniforms =====
uniform sampler2D u_HDRTexture;
uniform sampler2D u_BloomTexture;
uniform float u_Exposure = 1.0;
uniform float u_AvgLuminance = 0.5;
uniform int u_Method = 3;  // 0=None, 1=Reinhard, 2=ACES, 3=Exposure
uniform float u_Gamma = 2.2;  // Gamma correction value (2.2 = standard sRGB)
uniform int u_BloomEnabled = 0;
uniform float u_BloomStrength = 0.04;

// ===== Tone Mapping Operators =====

// ACES Filmic Tone Mapping
// Source: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Simple Reinhard Tone Mapping
vec3 Reinhard(vec3 hdrColor) {
    return hdrColor / (hdrColor + vec3(1.0));
}

// Reinhard with White Point (from ogldev)
vec3 Reinhard2(vec3 hdrColor, float white) {
    return (hdrColor * (1.0 + hdrColor / (white * white))) / (1.0 + hdrColor);
}

// Exposure-Based Tone Mapping (ogldev's default method)
vec3 ExposureToneMapping(vec3 hdrColor, float exposure) {
    // Apply exposure
    vec3 mapped = hdrColor * exposure;

    // Reinhard tone mapping
    mapped = mapped / (mapped + vec3(1.0));

    return mapped;
}

// Gamma Correction (Linear � sRGB)
vec3 GammaCorrection(vec3 color, float gamma) {
    return pow(color, vec3(1.0 / gamma));
}

// ===== Main =====
void main()
{
    // Sample HDR color
    vec3 hdrColor = texture(u_HDRTexture, TexCoords).rgb;

    // Add bloom if enabled (BEFORE tone mapping!)
    if (u_BloomEnabled == 1) {
        vec3 bloomColor = texture(u_BloomTexture, TexCoords).rgb;
        hdrColor += bloomColor * u_BloomStrength;
    }

    vec3 mapped;

    // Apply tone mapping based on selected method
    switch (u_Method) {
        case 0:  // No tone mapping (pass-through)
            mapped = hdrColor;
            break;

        case 1:  // Simple Reinhard
            mapped = Reinhard(hdrColor);
            break;

        case 2:  // ACES filmic (cinematic look)
            mapped = ACESFilm(hdrColor);
            break;

        case 3:  // Exposure + Reinhard (ogldev default)
            mapped = ExposureToneMapping(hdrColor, u_Exposure);
            break;

        default:
            mapped = Reinhard(hdrColor);
    }

    // Apply gamma correction if gamma > 0.0
    if (u_Gamma > 0.0) {
        mapped = GammaCorrection(mapped, u_Gamma);
    }

    FragColor = vec4(mapped, 1.0);
}
