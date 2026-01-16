#version 450 core

// Inputs from vertex shader
in vec2 vTexCoord;
in vec4 vColor;
in vec4 vOutlineColor;
in vec4 vGlowColor;
in float vSDFThreshold;
in float vSmoothing;
in float vOutlineWidth;
in float vGlowStrength;

// Uniforms
uniform sampler2D u_SDFAtlas;      // SDF atlas texture
uniform bool u_MultiChannelSDF;     // Whether atlas uses MSDF (RGB) or SDF (R)

// Output
out vec4 FragColor;

// SDF median function for multi-channel SDF (MSDF)
float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

// Sample SDF distance from atlas
float sampleSDF(vec2 uv)
{
    if (u_MultiChannelSDF)
    {
        // Multi-channel SDF (better quality for complex glyphs)
        vec3 sdfSample = texture(u_SDFAtlas, uv).rgb;
        return median(sdfSample.r, sdfSample.g, sdfSample.b);
    }
    else
    {
        // Single-channel SDF
        return texture(u_SDFAtlas, uv).r;
    }
}

void main()
{
    // Sample SDF distance field
    float distance = sampleSDF(vTexCoord);

    // Calculate main text alpha with anti-aliasing
    float alpha = smoothstep(vSDFThreshold - vSmoothing, vSDFThreshold + vSmoothing, distance);

    // Start with base text color
    vec4 finalColor = vColor;
    float finalAlpha = alpha * vColor.a;

    // Apply outline effect if enabled
    if (vOutlineWidth > 0.0)
    {
        // Calculate outline threshold (inner edge of outline)
        float outlineThreshold = vSDFThreshold - vOutlineWidth;
        float outlineAlpha = smoothstep(outlineThreshold - vSmoothing, outlineThreshold + vSmoothing, distance);

        // Mix outline with text color
        // Outline is visible where outlineAlpha > 0 but alpha == 0
        finalColor = mix(vOutlineColor, vColor, alpha);
        finalAlpha = max(outlineAlpha * vOutlineColor.a, finalAlpha);
    }

    // Apply glow/shadow effect if enabled
    if (vGlowStrength > 0.0)
    {
        // Calculate glow threshold (outer edge of glow)
        float glowThreshold = vSDFThreshold - vGlowStrength;
        float glowAlpha = smoothstep(glowThreshold - vSmoothing * 2.0, glowThreshold + vSmoothing * 2.0, distance);

        // Mix glow under the text/outline
        finalColor.rgb = mix(vGlowColor.rgb, finalColor.rgb, alpha + (1.0 - alpha) * (1.0 - glowAlpha));
        finalAlpha = max(glowAlpha * vGlowColor.a, finalAlpha);
    }

    // Apply final alpha
    finalColor.a = finalAlpha;

    // Output final color
    FragColor = finalColor;

    // Discard fully transparent pixels
    if (FragColor.a < 0.01)
    {
        discard;
    }
}
