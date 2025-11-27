#version 450 core

// Vertex attributes (quad mesh)
layout(location = 0) in vec2 aPosition;  // Local quad position (0,0 to 1,1)
layout(location = 1) in vec2 aTexCoord;  // Texture coordinates

// Instance data structure (must be defined outside the buffer block)
struct HUDInstanceData
{
    vec2 position;      // Screen position (pixels)
    vec2 size;          // Size (pixels)
    vec4 color;         // Tint color
    float rotation;     // Rotation (radians)
    uint anchor;        // Anchor point (0-8)
    float padding[2];   // Alignment padding
};

// Instance data from SSBO
layout(std430, binding = 3) buffer HUDInstanceBuffer
{
    HUDInstanceData instances[];
};

// Uniforms
uniform vec2 u_ViewportSize;       // Actual viewport width and height in pixels
uniform vec2 u_ReferenceResolution;  // Fixed reference resolution (e.g., 1920x1080) for HUD layout

// Outputs
out vec2 vTexCoord;
out vec4 vColor;

// Anchor point enum (matches C++ HUDAnchor)
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
    return vec2(0.0, 0.0);  // Default
}

void main()
{
    // Get instance data
    HUDInstanceData instance = instances[gl_InstanceID];

    // Calculate anchor offset
    vec2 anchorOffset = CalculateAnchorOffset(instance.anchor);

    // Offset vertex position by anchor (so rotation happens around anchor point)
    vec2 localPos = aPosition - anchorOffset;

    // Apply rotation around anchor point
    float c = cos(instance.rotation);
    float s = sin(instance.rotation);
    vec2 rotatedPos = vec2(
        localPos.x * c - localPos.y * s,
        localPos.x * s + localPos.y * c
    );

    // Scale by size (in pixels)
    vec2 scaledPos = rotatedPos * instance.size;

    // Translate to screen position (in pixels, origin at top-left)
    vec2 screenPos = instance.position + scaledPos;

    // Convert from pixel coordinates to NDC
    // HUD positions are in reference resolution space (e.g., 1920x1080)
    // This ensures HUD elements maintain fixed pixel sizes regardless of viewport
    // OpenGL NDC: (-1,-1) = bottom-left, (1,1) = top-right
    // Screen coords: (0,0) = top-left, (width,height) = bottom-right
    vec2 ndc;
    ndc.x = (screenPos.x / u_ReferenceResolution.x) * 2.0 - 1.0;  // 0..refWidth -> -1..1
    ndc.y = 1.0 - (screenPos.y / u_ReferenceResolution.y) * 2.0;  // 0..refHeight -> 1..-1 (flip Y)

    gl_Position = vec4(ndc, 0.0, 1.0);

    // Pass texture coordinates and color to fragment shader
    vTexCoord = aTexCoord;
    vColor = instance.color;
}
