#version 450 core

// Vertex attributes (quad mesh)
layout(location = 0) in vec2 aPosition;  // Local quad position (0,0 to 1,1)
layout(location = 1) in vec2 aTexCoord;  // Texture coordinates (will be replaced by glyph UVs)

// World-space glyph instance data structure
struct WorldGlyphInstanceData
{
    vec3 worldPosition;     // 3D world position
    float _padding0;        // Alignment
    vec3 billboardRight;    // Billboard right vector (pre-calculated on CPU)
    float _padding1;        // Alignment
    vec3 billboardUp;       // Billboard up vector (pre-calculated on CPU)
    float _padding2;        // Alignment
    vec2 size;              // Glyph size in world units
    vec2 _padding3;         // Alignment
    vec4 uvRect;            // Atlas UV coordinates (x=min.x, y=min.y, z=max.x, w=max.y)
    vec4 color;             // Text color (RGBA)
    float sdfThreshold;     // SDF cutoff threshold (0.5 default)
    float smoothing;        // Edge smoothing factor (0.04 default)
    float outlineWidth;     // Outline thickness
    float glowStrength;     // Glow/shadow strength
    vec4 outlineColor;      // Outline color (RGBA)
    vec4 glowColor;         // Glow/shadow color (RGBA)
    // Total: 144 bytes
};

// Instance data from SSBO
layout(std430, binding = 4) buffer WorldGlyphInstanceBuffer
{
    WorldGlyphInstanceData instances[];
};

// Camera uniforms
uniform mat4 u_View;        // View matrix
uniform mat4 u_Projection;  // Projection matrix

// Outputs to fragment shader
out vec2 vTexCoord;
out vec4 vColor;
out vec4 vOutlineColor;
out vec4 vGlowColor;
out float vSDFThreshold;
out float vSmoothing;
out float vOutlineWidth;
out float vGlowStrength;

void main()
{
    // Get instance data
    WorldGlyphInstanceData instance = instances[gl_InstanceID];

    // Build world-space quad position using billboard basis vectors
    // aPosition is in (0,1) range, anchored at top-left (0,0)
    // This matches screen-space text behavior for correct bearing application
    vec2 localPos = aPosition;

    // Build world position using billboard right/up vectors
    // Negate Y because quad vertices use Y-down convention (0=top, 1=bottom)
    // but billboardUp points up in world space
    vec3 worldPos = instance.worldPosition
        + instance.billboardRight * (localPos.x * instance.size.x)
        + instance.billboardUp * (-localPos.y * instance.size.y);

    // Apply MVP transformation
    gl_Position = u_Projection * u_View * vec4(worldPos, 1.0);

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
