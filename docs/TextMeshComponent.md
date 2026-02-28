# TextMeshComponent Reference

## Overview

The `TextMeshComponent` is used for rendering text in 3D world-space, similar to Unity's TextMesh component. It provides comprehensive control over text appearance, layout, and orientation in the game world.

**Location**: `engine/include/Render/Render.h:207-258`

---

## Component Dependencies

The TextMeshComponent requires:
- **TransformComponent**: For world position (where text appears)
- **TransformMtxComponent**: For rotation matrix (used in billboard mode None)
- **Font Atlas Asset**: Referenced by `m_FontGuid`

---

## Data Members

### Font Asset

#### `m_FontGuid`
```cpp
rp::BasicIndexedGuid m_FontGuid
```

| Property | Value |
|----------|-------|
| **Type** | GUID (Globally Unique Identifier) |
| **Default** | Points to default "font_atlas" type |
| **Purpose** | References which font atlas asset to use for rendering |

**Description**: The engine uses this GUID to load the font texture atlas and glyph metrics.

**Related**: Font atlases are created by the font importer (`lib/resource/ext/include/importer/font.hpp`)

---

### Text Content

#### `text`
```cpp
std::string text = "World Text"
```

| Property | Value |
|----------|-------|
| **Type** | String (UTF-8 encoded) |
| **Default** | `"World Text"` |
| **Purpose** | The actual text content to render |

**Description**: Supports UTF-8 encoding for international characters.

**Example**:
```cpp
textMesh.text = "Hello 世界";  // English + Chinese characters
```

---

### Billboard Mode

#### `billboardMode`
```cpp
enum class BillboardMode {
    None,         // Uses entity's transform rotation (fixed orientation)
    Full,         // Always faces camera (rotates on all axes)
    Cylindrical   // Rotates horizontally but stays upright (Y-axis only)
} billboardMode = BillboardMode::Full;
```

| Property | Value |
|----------|-------|
| **Type** | Enum (3 options) |
| **Default** | `Full` |
| **Purpose** | Controls how text orients toward the camera |

**Options**:
- **`None`**: Uses entity's transform rotation (fixed orientation)
- **`Full`**: Always faces camera completely (rotates on all axes)
- **`Cylindrical`**: Rotates horizontally but stays upright (Y-axis only)

**See Also**: [`docs/billboard_modes.md`](billboard_modes.md) for detailed explanation

**Example**:
```cpp
// Name tag that always faces player
textMesh.billboardMode = TextMeshComponent::BillboardMode::Full;

// Street sign that rotates horizontally but stays upright
textMesh.billboardMode = TextMeshComponent::BillboardMode::Cylindrical;

// Text on a wall that doesn't rotate
textMesh.billboardMode = TextMeshComponent::BillboardMode::None;
```

---

### Text Sizing Mode

#### `sizingMode`
```cpp
enum class TextSizingMode {
    DistanceScaled,  // Text scales with distance (original Unity-style behavior)
    ScreenConstant   // Text maintains constant screen-space size (like HUD text)
} sizingMode = TextSizingMode::ScreenConstant;
```

| Property | Value |
|----------|-------|
| **Type** | Enum (2 options) |
| **Default** | `ScreenConstant` |
| **Purpose** | Controls how text size changes with camera distance |

**Options**:
- **`DistanceScaled`**: Text scales with perspective (appears smaller when farther away, larger when closer) - Unity-style behavior using `referenceDistance`
- **`ScreenConstant`**: Text maintains constant screen-space pixel size regardless of distance - like HUD/UI overlays in 3D space

**Formulas**:

**DistanceScaled mode**:
```
actualSize = (fontSize * referenceDistance) / distanceToCamera
```

**ScreenConstant mode**:
```
actualSize = (fontSize / screenHeight) * (2 * distance * tan(FOV/2))
```

**Comparison**:

| Distance | DistanceScaled (fontSize=16, refDist=10) | ScreenConstant (fontSize=16) |
|----------|------------------------------------------|------------------------------|
| 5 units  | 32 pixels (2× larger)                   | 16 pixels (constant)         |
| 10 units | 16 pixels (reference)                   | 16 pixels (constant)         |
| 20 units | 8 pixels (2× smaller)                   | 16 pixels (constant)         |

**Use Cases**:
- **ScreenConstant**: Player name tags, damage numbers, UI labels that should always be readable
- **DistanceScaled**: Environmental text, signs that should feel part of the world

**Example**:
```cpp
// Player name tag - always readable regardless of distance
textMesh.sizingMode = TextMeshComponent::TextSizingMode::ScreenConstant;
textMesh.fontSize = 18.0f;  // Always 18 pixels on screen

// World sign - scales naturally with perspective
textMesh.sizingMode = TextMeshComponent::TextSizingMode::DistanceScaled;
textMesh.fontSize = 24.0f;
textMesh.referenceDistance = 15.0f;  // 24 pixels at 15 units away
```

---

### Font Sizing Parameters

#### `fontSize`
```cpp
float fontSize = 16.0f;
```

| Property | Value |
|----------|-------|
| **Type** | Float (pixels) |
| **Default** | `16.0` |
| **Purpose** | Font size in pixels |

**Description**:
- In **ScreenConstant mode**: Text always appears at exactly this pixel size on screen
- In **DistanceScaled mode**: Text appears at this pixel size when camera is at `referenceDistance`

**Example**:
```cpp
textMesh.fontSize = 32.0f;  // 32 pixels on screen (ScreenConstant), or 32 pixels at refDist (DistanceScaled)
```

#### `referenceDistance`
```cpp
float referenceDistance = 10.0f;
```

| Property | Value |
|----------|-------|
| **Type** | Float (world units) |
| **Default** | `10.0` |
| **Purpose** | Distance at which `fontSize` is accurate **(only used in DistanceScaled mode)** |

**Description**: Controls distance-based scaling when `sizingMode = DistanceScaled`. Ignored in ScreenConstant mode.

**Formula** (DistanceScaled mode only):
```
actualSize = (fontSize * referenceDistance) / distanceToCamera
```

**Behavior** (DistanceScaled mode):
- At 10 units away: Text is 16 pixels tall
- At 5 units away: Text is 32 pixels tall (2× larger)
- At 20 units away: Text is 8 pixels tall (2× smaller)

**Example**:
```cpp
textMesh.sizingMode = TextMeshComponent::TextSizingMode::DistanceScaled;
textMesh.fontSize = 24.0f;
textMesh.referenceDistance = 15.0f;
// Text will be 24 pixels tall when camera is 15 units away
```

---

### Text Alignment

#### `alignment`
```cpp
enum class Alignment {
    Left,
    Center,
    Right,
    Justified  // Not yet fully implemented
} alignment = Alignment::Center;
```

| Property | Value |
|----------|-------|
| **Type** | Enum (4 options) |
| **Default** | `Center` |
| **Purpose** | Horizontal alignment for multi-line text |

**Options**:
- **`Left`**: Text aligns to left edge
- **`Center`**: Text centers on entity position
- **`Right`**: Text aligns to right edge
- **`Justified`**: Stretches text to fill width (not yet implemented)

**Note**: Alignment still affects single-line text positioning (centered at entity position vs left-aligned).

**Example**:
```cpp
// Center text on entity (good for floating labels)
textMesh.alignment = TextMeshComponent::Alignment::Center;

// Left-align text (good for multi-line descriptions)
textMesh.alignment = TextMeshComponent::Alignment::Left;
```

---

### Layout Properties

#### `lineSpacing`
```cpp
float lineSpacing = 1.0f;
```

| Property | Value |
|----------|-------|
| **Type** | Float (multiplier) |
| **Default** | `1.0` |
| **Purpose** | Controls vertical space between lines |

**Range**:
- `0.5` = Half spacing (tight lines)
- `1.0` = Normal spacing
- `2.0` = Double spacing

**Example**:
```
lineSpacing = 1.0:      lineSpacing = 2.0:
Hello                   Hello
World
                        World
```

```cpp
textMesh.lineSpacing = 1.5f;  // 1.5× line height
```

#### `letterSpacing`
```cpp
float letterSpacing = 0.0f;
```

| Property | Value |
|----------|-------|
| **Type** | Float (world units) |
| **Default** | `0.0` |
| **Purpose** | Extra horizontal space between characters |

**Units**: World units (affected by distance scaling)

**Example**:
- `0.0`: `Hello` (normal spacing)
- `0.1`: `H e l l o` (spaced out)
- `-0.05`: `Hello` (tighter than normal)

```cpp
textMesh.letterSpacing = 0.05f;  // Slightly spaced out
```

#### `maxWidth`
```cpp
float maxWidth = 0.0f;
```

| Property | Value |
|----------|-------|
| **Type** | Float (world units) |
| **Default** | `0.0` |
| **Purpose** | Maximum width before word wrapping |

**Behavior**:
- `0.0`: Text stays on one line
- `> 0.0`: Text wraps to next line when exceeding this width

**Example**: `maxWidth = 5.0` might wrap "Hello World" to:
```
Hello
World
```

```cpp
textMesh.maxWidth = 10.0f;  // Wrap text at 10 world units width
textMesh.text = "This is a long line of text that will wrap";
```

---

### Visual Properties

#### `color`
```cpp
glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
```

| Property | Value |
|----------|-------|
| **Type** | RGBA color (4 floats) |
| **Default** | White `(1, 1, 1, 1)` |
| **Purpose** | Base text color |

**Format**: `(Red, Green, Blue, Alpha)` - values from 0.0 to 1.0

**Examples**:
```cpp
textMesh.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);   // Red
textMesh.color = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);   // White at 50% opacity
textMesh.color = glm::vec4(0.2f, 0.8f, 0.2f, 1.0f);   // Green
```

---

### Outline Effect

#### `outlineWidth`
```cpp
float outlineWidth = 0.0f;
```

| Property | Value |
|----------|-------|
| **Type** | Float |
| **Default** | `0.0` (no outline) |
| **Purpose** | Thickness of outline/stroke around text |

**Range**: `0.0` to `~0.5` (relative to SDF range)

**Effect**: Creates a border around each letter

**Note**: Scaled by distance scaling

**Example**:
```cpp
textMesh.outlineWidth = 0.1f;  // Medium outline
```

#### `outlineColor`
```cpp
glm::vec4 outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
```

| Property | Value |
|----------|-------|
| **Type** | RGBA color |
| **Default** | Black `(0, 0, 0, 1)` |
| **Purpose** | Color of the outline |

**Common Use**: Black outline on white text for readability over any background

**Example**:
```cpp
// White text with black outline (high contrast)
textMesh.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
textMesh.outlineWidth = 0.15f;
textMesh.outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
```

---

### Glow/Shadow Effect

#### `glowStrength`
```cpp
float glowStrength = 0.0f;
```

| Property | Value |
|----------|-------|
| **Type** | Float |
| **Default** | `0.0` (no glow) |
| **Purpose** | Intensity of glow/drop shadow effect |

**Range**: `0.0` to `~1.0`

**Effect**: Creates a soft shadow or glow around text

**Example**:
```cpp
textMesh.glowStrength = 0.5f;  // Medium shadow/glow
```

#### `glowColor`
```cpp
glm::vec4 glowColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.5f);
```

| Property | Value |
|----------|-------|
| **Type** | RGBA color |
| **Default** | Semi-transparent black `(0, 0, 0, 0.5)` |
| **Purpose** | Color of the glow/shadow |

**Default Effect**: Creates drop shadow

**Example Uses**:
```cpp
// Black drop shadow
textMesh.glowStrength = 0.6f;
textMesh.glowColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.5f);

// Yellow glow effect
textMesh.glowStrength = 0.8f;
textMesh.glowColor = glm::vec4(1.0f, 1.0f, 0.0f, 0.8f);
```

---

### SDF Rendering Parameters

#### `sdfThreshold`
```cpp
float sdfThreshold = 0.5f;
```

| Property | Value |
|----------|-------|
| **Type** | Float |
| **Default** | `0.5` |
| **Purpose** | Cutoff threshold for SDF (Signed Distance Field) rendering |

**Range**: `0.0` to `1.0`

**Effect**:
- `0.5`: Normal, sharp text edges
- `< 0.5`: Makes text thicker/bolder
- `> 0.5`: Makes text thinner

**Technical**: Distance field value at which a pixel is considered "inside" the glyph

**Example**:
```cpp
textMesh.sdfThreshold = 0.45f;  // Slightly bolder text
textMesh.sdfThreshold = 0.55f;  // Slightly thinner text
```

#### `smoothing`
```cpp
float smoothing = 0.04f;
```

| Property | Value |
|----------|-------|
| **Type** | Float |
| **Default** | `0.04` |
| **Purpose** | Edge anti-aliasing smoothness |

**Range**: `0.0` to `~0.2`

**Effect**:
- `0.0`: Hard aliased edges (pixelated)
- `0.04`: Smooth anti-aliased edges (default)
- `> 0.1`: Very blurry edges

**Technical**: Controls width of the alpha gradient at text edges

**Example**:
```cpp
textMesh.smoothing = 0.02f;  // Sharper edges
textMesh.smoothing = 0.08f;  // Softer edges
```

---

### Rendering Control

#### `visible`
```cpp
bool visible = true;
```

| Property | Value |
|----------|-------|
| **Type** | Boolean |
| **Default** | `true` |
| **Purpose** | Controls whether text is rendered |

**Usage**: Toggle visibility without removing component

**Example**:
```cpp
// Hide tutorial text after completion
if (playerCompletedTutorial) {
    tutorialText.visible = false;
}
```

---

## Complete Usage Example

```cpp
// Create entity with text
ecs::entity textEntity = world.add_entity();
textEntity.add<TransformComponent>();
textEntity.add<TransformMtxComponent>();

auto& textMesh = textEntity.add<TextMeshComponent>();

// Configure text content
textMesh.text = "Hello World";

// Configure size and scaling
textMesh.sizingMode = TextMeshComponent::TextSizingMode::ScreenConstant;  // Constant screen size
textMesh.fontSize = 24.0f;

// Configure orientation
textMesh.billboardMode = TextMeshComponent::BillboardMode::Full;

// Configure layout
textMesh.alignment = TextMeshComponent::Alignment::Center;
textMesh.lineSpacing = 1.2f;
textMesh.letterSpacing = 0.0f;

// Style the text (white with black outline)
textMesh.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
textMesh.outlineWidth = 0.1f;
textMesh.outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

// Add drop shadow
textMesh.glowStrength = 0.5f;
textMesh.glowColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.5f);

// Position in world
auto& transform = textEntity.get<TransformComponent>();
transform.m_Translation = glm::vec3(0, 5, 0);  // 5 units above origin
```

---

## Common Use Cases

### Player Name Tag
```cpp
textMesh.text = playerName;
textMesh.sizingMode = TextMeshComponent::TextSizingMode::ScreenConstant;  // Always readable
textMesh.fontSize = 18.0f;
textMesh.billboardMode = TextMeshComponent::BillboardMode::Full;
textMesh.alignment = TextMeshComponent::Alignment::Center;
textMesh.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
textMesh.outlineWidth = 0.12f;
textMesh.outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
```

### Damage Number
```cpp
textMesh.text = "-50";
textMesh.sizingMode = TextMeshComponent::TextSizingMode::ScreenConstant;  // Consistent visibility
textMesh.fontSize = 32.0f;
textMesh.billboardMode = TextMeshComponent::BillboardMode::Full;
textMesh.color = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);  // Red
textMesh.glowStrength = 0.7f;
textMesh.glowColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.8f);
```

### Location Sign
```cpp
textMesh.text = "Town Square";
textMesh.sizingMode = TextMeshComponent::TextSizingMode::DistanceScaled;  // Feels part of world
textMesh.fontSize = 28.0f;
textMesh.referenceDistance = 20.0f;
textMesh.billboardMode = TextMeshComponent::BillboardMode::Cylindrical;
textMesh.alignment = TextMeshComponent::Alignment::Center;
textMesh.color = glm::vec4(0.9f, 0.8f, 0.6f, 1.0f);  // Beige
textMesh.outlineWidth = 0.08f;
```

### Wall Text (No Billboard)
```cpp
textMesh.text = "Emergency Exit";
textMesh.sizingMode = TextMeshComponent::TextSizingMode::DistanceScaled;  // Natural perspective
textMesh.fontSize = 20.0f;
textMesh.referenceDistance = 5.0f;
textMesh.billboardMode = TextMeshComponent::BillboardMode::None;
textMesh.color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);  // Green
```

---

## Related Systems

- **RenderSystem** (`engine/src/Render/Render.cpp`): Processes TextMeshComponent and submits to TextRenderer
- **TextRenderer** (`lib/Graphics/src/Rendering/TextRenderer.cpp`): CPU-side text layout and GPU submission
- **Font Importer** (`lib/resource/ext/include/importer/font.hpp`): Creates font atlas assets from TTF/OTF files
- **Shaders**:
  - `bin/assets/shaders/text_sdf_world.vert`: Vertex shader for world text
  - `bin/assets/shaders/text_sdf_world.frag`: Fragment shader with SDF rendering

---

## Performance Considerations

### Text Updates
- Changing `text` content triggers re-layout (expensive)
- Changing visual properties (color, outline, etc.) is cheap
- Minimize per-frame text content changes

### Distance Scaling
- **ScreenConstant mode** (default): Text maintains constant pixel size, no distance calculations
- **DistanceScaled mode**: Uses Unity-style `fontSize * referenceDistance / distance` formula
- ScreenConstant mode is slightly more expensive (requires FOV and distance calculations)
- No LOD system - all glyphs rendered regardless of distance

### Billboard Calculations
- **None**: Cheapest (no calculations)
- **Cylindrical**: Low cost (1 cross product per frame)
- **Full**: Medium cost (2 cross products per frame)

---

## See Also

- [Billboard Modes Documentation](billboard_modes.md)
- [CLAUDE.md](../CLAUDE.md) - Project architecture overview
- Font Atlas Documentation (TBD)
