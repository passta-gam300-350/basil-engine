#version 450 core

// Vertex attributes (quad mesh)
layout(location = 0) in vec2 aPosition;  // Local quad position (0,0 to 1,1)
layout(location = 1) in vec2 aTexCoord;  // Texture coordinates (will be replaced by glyph UVs)

// Glyph instance data structure
struct GlyphInstanceData
{
    vec2 position;          // Screen position (pixels)
    vec2 size;              // Glyph size (pixels)
    vec4 uvRect;            // Atlas UV coordinates (x=min.x, y=min.y, z=max.x, w=max.y)
    vec4 color;             // Text color (RGBA)
    float sdfThreshold;     // SDF cutoff threshold (0.5 default)
    float smoothing;        // Edge smoothing factor (0.04 default)
    float outlineWidth;     // Outline thickness (0.0 = no outline)
    float glowStrength;     // Glow/shadow strength (0.0 = no glow)
    vec4 outlineColor;      // Outline color (RGBA)
    vec4 glowColor;         // Glow/shadow color (RGBA)
    float rotation;         // Rotation (radians)
    uint anchor;            // Anchor point (0-8)
    float padding[2];       // Alignment padding to 128 bytes
};

// Instance data from SSBO
layout(std430, binding = 4) buffer GlyphInstanceBuffer
{
    GlyphInstanceData instances[];
};

// Uniforms
uniform vec2 u_ViewportSize;         // Actual viewport width and height in pixels
uniform vec2 u_ReferenceResolution;  // Fixed reference resolution (e.g., 1920x1080)

// Outputs
out vec2 vTexCoord;
out vec4 vColor;
out vec4 vOutlineColor;
out vec4 vGlowColor;
out float vSDFThreshold;
out float vSmoothing;
out float vOutlineWidth;
out float vGlowStrength;

// Anchor point enum (matches C++ TextAnchor)
const uint ANCHOR_TOP_LEFT = 0u;
const uint ANCHOR_TOP_CENTER = 1u;
const uint ANCHOR_TOP_RIGHT = 2u;
const uint ANCHOR_CENTER_LEFT = 3u;
const uint ANCHOR_CENTER = 4u;
const uint ANCHOR_CENTER_RIGHT = 5u;
const uint ANCHOR_BOTTOM_LEFT = 6u;
const uint ANCHOR_BOTTOM_CENTER = 7u;
const uint ANCHOR_BOTTOM_RIGHT = 8u;

// Calculate anchor offset (normalized 0-1 range within quad)
// Y-down coordinate system: Y=0 at top, Y=1 at bottom
vec2 CalculateAnchorOffset(uint anchor)
{
    if (anchor == ANCHOR_TOP_LEFT)      return vec2(0.0, 0.0);
    if (anchor == ANCHOR_TOP_CENTER)    return vec2(0.5, 0.0);
    if (anchor == ANCHOR_TOP_RIGHT)     return vec2(1.0, 0.0);
    if (anchor == ANCHOR_CENTER_LEFT)   return vec2(0.0, 0.5);
    if (anchor == ANCHOR_CENTER)        return vec2(0.5, 0.5);
    if (anchor == ANCHOR_CENTER_RIGHT)  return vec2(1.0, 0.5);
    if (anchor == ANCHOR_BOTTOM_LEFT)   return vec2(0.0, 1.0);
    if (anchor == ANCHOR_BOTTOM_CENTER) return vec2(0.5, 1.0);
    if (anchor == ANCHOR_BOTTOM_RIGHT)  return vec2(1.0, 1.0);
    return vec2(0.0, 0.0);  // Default (top-left)
}

void main()
{
    // Get instance data
    GlyphInstanceData instance = instances[gl_InstanceID];

    // Calculate anchor offset
    vec2 anchorOffset = CalculateAnchorOffset(instance.anchor);

    // Offset vertex position by anchor
    vec2 localPos = aPosition - anchorOffset;

    // Apply rotation around anchor point
    float c = cos(instance.rotation);
    float s = sin(instance.rotation);
    vec2 rotatedPos = vec2(
        localPos.x * c - localPos.y * s,
        localPos.x * s + localPos.y * c
    );

    // Scale by glyph size (in pixels)
    vec2 scaledPos = rotatedPos * instance.size;

    // Translate to screen position (in pixels, origin at top-left)
    vec2 screenPos = instance.position + scaledPos;

    // Convert from pixel coordinates to NDC
    vec2 ndc;
    ndc.x = (screenPos.x / u_ReferenceResolution.x) * 2.0 - 1.0;
    ndc.y = 1.0 - (screenPos.y / u_ReferenceResolution.y) * 2.0;

    gl_Position = vec4(ndc, 0.0, 1.0);

    // Map vertex UV (0-1) to glyph UV rect in atlas
    vec2 uvMin = instance.uvRect.xy;
    vec2 uvMax = instance.uvRect.zw;
    vTexCoord = mix(uvMin, uvMax, aTexCoord);

    // Pass effect parameters to fragment shader
    vColor = instance.color;
    vOutlineColor = instance.outlineColor;
    vGlowColor = instance.glowColor;
    vSDFThreshold = instance.sdfThreshold;
    vSmoothing = instance.smoothing;
    vOutlineWidth = instance.outlineWidth;
    vGlowStrength = instance.glowStrength;
}
