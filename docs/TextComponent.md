# TextComponent Reference

## Overview

The `TextComponent` is used for rendering text in **screen-space (2D UI)**, similar to Unity's UI Text component. It provides comprehensive control over text appearance, layout, and positioning on the screen using pixel-based coordinates.

**Location**: `engine/include/Render/Render.h:141-200`

---

## Key Differences from TextMeshComponent

| Feature | **TextComponent** (Screen/UI) | **TextMeshComponent** (World/3D) |
|---------|------------------------------|----------------------------------|
| **Space** | Screen-space (2D, pixels) | World-space (3D, world units) |
| **Positioning** | Pixel coordinates + anchor | Transform component (world position) |
| **Billboard** | N/A (always faces screen) | Full, Cylindrical, None |
| **Distance Scaling** | None (fixed pixel size) | Yes (Unity-style distance scaling) |
| **Rotation** | 2D rotation (degrees) | 3D rotation (from transform) |
| **Use Case** | HUD, menus, UI overlays | In-world labels, signs, name tags |

---

## Component Dependencies

The TextComponent requires:
- **Font Atlas Asset**: Referenced by `m_FontGuid`
- **No Transform Component**: Uses screen-space positioning

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
std::string text = "Text"
```

| Property | Value |
|----------|-------|
| **Type** | String (UTF-8 encoded) |
| **Default** | `"Text"` |
| **Purpose** | The actual text content to render |

**Description**: Supports UTF-8 encoding for international characters.

**Example**:
```cpp
textComponent.text = "Score: 1000";
textComponent.text = "你好世界";  // Chinese characters
textComponent.text = "Здравствуй";  // Cyrillic
```

---

### Screen-Space Positioning

#### `position`
```cpp
glm::vec2 position = glm::vec2(0.0f)
```

| Property | Value |
|----------|-------|
| **Type** | 2D Vector (pixels) |
| **Default** | `(0, 0)` |
| **Purpose** | Position offset in pixels from the anchor point |

**Description**: Specifies where the text appears on screen, relative to the chosen anchor point.

**Coordinate System**:
- **Origin**: Depends on anchor (see below)
- **X-axis**: Left to right (positive = right)
- **Y-axis**: Top to bottom (positive = down)

**Example**:
```cpp
// Position text 100 pixels right and 50 pixels down from anchor
textComponent.position = glm::vec2(100.0f, 50.0f);

// Position text slightly left and up from anchor
textComponent.position = glm::vec2(-20.0f, -10.0f);
```

#### `fontSize`
```cpp
float fontSize = 16.0f
```

| Property | Value |
|----------|-------|
| **Type** | Float (pixels) |
| **Default** | `16.0` |
| **Purpose** | Font size in screen pixels |

**Description**: Unlike TextMeshComponent, this is a **fixed pixel size** with no distance scaling.

**Example**:
```cpp
textComponent.fontSize = 12.0f;  // Small text (like tooltips)
textComponent.fontSize = 24.0f;  // Medium text (like buttons)
textComponent.fontSize = 48.0f;  // Large text (like titles)
```

---

### Anchor System

#### `anchor`
```cpp
enum class Anchor {
    TopLeft, TopCenter, TopRight,
    CenterLeft, Center, CenterRight,
    BottomLeft, BottomCenter, BottomRight
} anchor = Anchor::TopLeft;
```

| Property | Value |
|----------|-------|
| **Type** | Enum (9 options) |
| **Default** | `TopLeft` |
| **Purpose** | Defines the origin point for positioning on screen |

**Description**: The anchor determines which point on the screen is used as the origin (0, 0) for the `position` offset.

**Anchor Points**:
```
TopLeft ─────── TopCenter ─────── TopRight
   │                                  │
   │                                  │
CenterLeft ───── Center ───── CenterRight
   │                                  │
   │                                  │
BottomLeft ── BottomCenter ── BottomRight
```

**Examples**:

**TopLeft Anchor** (default):
```cpp
textComponent.anchor = TextComponent::Anchor::TopLeft;
textComponent.position = glm::vec2(10, 10);
// Result: Text at top-left corner, 10px from edges
```

**Center Anchor**:
```cpp
textComponent.anchor = TextComponent::Anchor::Center;
textComponent.position = glm::vec2(0, 0);
// Result: Text perfectly centered on screen
```

**BottomRight Anchor**:
```cpp
textComponent.anchor = TextComponent::Anchor::BottomRight;
textComponent.position = glm::vec2(-10, -10);
// Result: Text at bottom-right, 10px from edges
```

**Common Use Cases**:
- `TopLeft`: Score displays, FPS counters
- `TopCenter`: Level names, objectives
- `TopRight`: Mini-map labels
- `Center`: Game over screens, main titles
- `BottomLeft`: Tutorial hints
- `BottomCenter`: Subtitles, prompts
- `BottomRight`: Build version, debug info

---

### Text Alignment

#### `alignment`
```cpp
enum class Alignment {
    Left, Center, Right, Justified
} alignment = Alignment::Left;
```

| Property | Value |
|----------|-------|
| **Type** | Enum (4 options) |
| **Default** | `Left` |
| **Purpose** | Horizontal alignment for multi-line text |

**Description**: Controls how text lines align relative to each other and the anchor point.

**Options**:
- **`Left`**: Lines align to the left edge
- **`Center`**: Lines are centered
- **`Right`**: Lines align to the right edge
- **`Justified`**: Lines stretch to fill width (not yet fully implemented)

**Visual Examples**:

**Left-aligned** (default):
```
Hello World
This is a longer line
End
```

**Center-aligned**:
```
   Hello World
This is a longer line
       End
```

**Right-aligned**:
```
            Hello World
 This is a longer line
                    End
```

**Example**:
```cpp
// Title screen (center-aligned)
textComponent.text = "GAME TITLE\nPress Start";
textComponent.alignment = TextComponent::Alignment::Center;
textComponent.anchor = TextComponent::Anchor::Center;

// Description text (left-aligned)
textComponent.text = "Inventory:\n- Sword\n- Shield\n- Potion";
textComponent.alignment = TextComponent::Alignment::Left;
```

---

### Layout Properties

#### `lineSpacing`
```cpp
float lineSpacing = 1.0f
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
```cpp
// Tight text (for compact displays)
textComponent.lineSpacing = 0.8f;

// Spacious text (for readability)
textComponent.lineSpacing = 1.5f;
```

**Visual**:
```
lineSpacing = 1.0:      lineSpacing = 2.0:
Line 1                  Line 1
Line 2
Line 3                  Line 2

                        Line 3
```

#### `letterSpacing`
```cpp
float letterSpacing = 0.0f
```

| Property | Value |
|----------|-------|
| **Type** | Float (pixels) |
| **Default** | `0.0` |
| **Purpose** | Extra horizontal space between characters |

**Units**: **Pixels** (not world units like TextMeshComponent)

**Example**:
```cpp
// Normal spacing
textComponent.letterSpacing = 0.0f;  // "HELLO"

// Spaced out (title effect)
textComponent.letterSpacing = 5.0f;  // "H  E  L  L  O"

// Tight spacing
textComponent.letterSpacing = -1.0f;  // "HELLO" (condensed)
```

#### `maxWidth`
```cpp
float maxWidth = 0.0f
```

| Property | Value |
|----------|-------|
| **Type** | Float (pixels) |
| **Default** | `0.0` |
| **Purpose** | Maximum width before word wrapping |

**Behavior**:
- `0.0`: No wrapping (single line, can extend off-screen)
- `> 0.0`: Text wraps to next line when exceeding this pixel width

**Example**:
```cpp
// Tooltip with max width
textComponent.text = "This is a long tooltip that needs to wrap";
textComponent.maxWidth = 200.0f;
textComponent.alignment = TextComponent::Alignment::Left;

// Result:
// This is a long
// tooltip that needs
// to wrap
```

---

### Visual Properties

#### `color`
```cpp
glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
```

| Property | Value |
|----------|-------|
| **Type** | RGBA color (4 floats) |
| **Default** | White `(1, 1, 1, 1)` |
| **Purpose** | Base text color |

**Format**: `(Red, Green, Blue, Alpha)` - values from 0.0 to 1.0

**Examples**:
```cpp
// White text
textComponent.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

// Red warning text
textComponent.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

// Green success message
textComponent.color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

// Semi-transparent overlay
textComponent.color = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);

// Gold/yellow (UI accent)
textComponent.color = glm::vec4(1.0f, 0.84f, 0.0f, 1.0f);
```

---

### Outline Effect

#### `outlineWidth`
```cpp
float outlineWidth = 0.0f
```

| Property | Value |
|----------|-------|
| **Type** | Float (pixels) |
| **Default** | `0.0` (no outline) |
| **Purpose** | Thickness of outline/stroke around text |

**Range**: `0.0` to `~0.5` (relative to SDF range)

**Units**: **Relative to font SDF** (not direct pixels)

**Example**:
```cpp
textComponent.outlineWidth = 0.1f;   // Thin outline
textComponent.outlineWidth = 0.2f;   // Medium outline
textComponent.outlineWidth = 0.3f;   // Thick outline
```

#### `outlineColor`
```cpp
glm::vec4 outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
```

| Property | Value |
|----------|-------|
| **Type** | RGBA color |
| **Default** | Black `(0, 0, 0, 1)` |
| **Purpose** | Color of the outline |

**Common Use**: Black or dark outline for maximum contrast and readability.

**Example**:
```cpp
// Classic white-on-black outline (high visibility)
textComponent.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
textComponent.outlineWidth = 0.15f;
textComponent.outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

// Gold text with dark brown outline
textComponent.color = glm::vec4(1.0f, 0.84f, 0.0f, 1.0f);
textComponent.outlineWidth = 0.12f;
textComponent.outlineColor = glm::vec4(0.3f, 0.2f, 0.0f, 1.0f);
```

---

### Glow/Shadow Effect

#### `glowStrength`
```cpp
float glowStrength = 0.0f
```

| Property | Value |
|----------|-------|
| **Type** | Float |
| **Default** | `0.0` (no glow) |
| **Purpose** | Intensity of glow/drop shadow effect |

**Range**: `0.0` to `~1.0`

**Example**:
```cpp
textComponent.glowStrength = 0.0f;   // No glow/shadow
textComponent.glowStrength = 0.3f;   // Subtle shadow
textComponent.glowStrength = 0.6f;   // Medium shadow
textComponent.glowStrength = 0.9f;   // Heavy glow
```

#### `glowColor`
```cpp
glm::vec4 glowColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.5f)
```

| Property | Value |
|----------|-------|
| **Type** | RGBA color |
| **Default** | Semi-transparent black `(0, 0, 0, 0.5)` |
| **Purpose** | Color of the glow/shadow |

**Default Effect**: Creates a drop shadow effect.

**Example Uses**:
```cpp
// Drop shadow (default)
textComponent.glowStrength = 0.5f;
textComponent.glowColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.6f);

// Blue magical glow
textComponent.glowStrength = 0.8f;
textComponent.glowColor = glm::vec4(0.3f, 0.5f, 1.0f, 0.7f);

// Red danger glow
textComponent.glowStrength = 0.7f;
textComponent.glowColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.5f);
```

**See Also**: [Outline vs Glow explanation](TextMeshComponent.md#outline-vs-glow-in-sdf-text-rendering)

---

### SDF Rendering Parameters

#### `sdfThreshold`
```cpp
float sdfThreshold = 0.5f
```

| Property | Value |
|----------|-------|
| **Type** | Float |
| **Default** | `0.5` |
| **Purpose** | Cutoff threshold for SDF (Signed Distance Field) rendering |

**Range**: `0.0` to `1.0`

**Effect**:
- `0.5`: Normal, sharp text edges
- `< 0.5`: Makes text thicker/bolder (e.g., `0.45`)
- `> 0.5`: Makes text thinner (e.g., `0.55`)

**Technical**: Distance field value at which a pixel is considered "inside" the glyph.

**Example**:
```cpp
textComponent.sdfThreshold = 0.5f;   // Normal weight
textComponent.sdfThreshold = 0.45f;  // Bolder
textComponent.sdfThreshold = 0.55f;  // Thinner
```

#### `smoothing`
```cpp
float smoothing = 0.04f
```

| Property | Value |
|----------|-------|
| **Type** | Float |
| **Default** | `0.04` |
| **Purpose** | Edge anti-aliasing smoothness |

**Range**: `0.0` to `~0.2`

**Effect**:
- `0.0`: Hard aliased edges (pixelated, retro look)
- `0.04`: Smooth anti-aliased edges (default, modern look)
- `> 0.1`: Very blurry edges

**Technical**: Controls width of the alpha gradient at text edges.

**Example**:
```cpp
textComponent.smoothing = 0.0f;    // Pixel-perfect retro text
textComponent.smoothing = 0.04f;   // Normal smooth text
textComponent.smoothing = 0.08f;   // Extra smooth (slightly blurry)
```

---

### Transform

#### `rotation`
```cpp
float rotation = 0.0f
```

| Property | Value |
|----------|-------|
| **Type** | Float (degrees) |
| **Default** | `0.0` |
| **Purpose** | 2D rotation around the anchor point |

**Direction**: **Clockwise** (positive = clockwise rotation)

**Example**:
```cpp
textComponent.rotation = 0.0f;     // No rotation
textComponent.rotation = 45.0f;    // 45° clockwise
textComponent.rotation = 90.0f;    // 90° clockwise (vertical text)
textComponent.rotation = -15.0f;   // 15° counter-clockwise
textComponent.rotation = 180.0f;   // Upside down
```

**Use Cases**:
- Angled labels
- Rotating indicators (e.g., "NEW!" banner)
- Vertical text
- Dynamic effects (rotating damage numbers)

#### `layer`
```cpp
uint8_t layer = 0
```

| Property | Value |
|----------|-------|
| **Type** | 8-bit unsigned integer |
| **Default** | `0` |
| **Purpose** | Layer for depth sorting |
| **Range** | `0` to `255` |

**Description**: Controls rendering order. **Higher values render on top** of lower values.

**Example**:
```cpp
// Background text
backgroundText.layer = 0;

// Normal UI text
uiText.layer = 10;

// Tooltip (appears above everything)
tooltip.layer = 100;

// Critical overlay (always on top)
errorMessage.layer = 255;
```

**Rendering Order**:
```
Layer 0   (bottom) ─────────┐
Layer 10                     │ Rendered first (back)
Layer 100                    │
Layer 255 (top)    ──────────┘ Rendered last (front)
```

#### `visible`
```cpp
bool visible = true
```

| Property | Value |
|----------|-------|
| **Type** | Boolean |
| **Default** | `true` |
| **Purpose** | Controls whether text is rendered |

**Usage**: Toggle visibility without removing component.

**Example**:
```cpp
// Show/hide tutorial text
if (tutorialComplete) {
    tutorialText.visible = false;
}

// Blinking effect
static float timer = 0.0f;
timer += deltaTime;
warningText.visible = (int(timer * 2) % 2 == 0);  // Blink every 0.5s
```

---

## Complete Usage Example

```cpp
// Create entity with screen-space text
ecs::entity textEntity = world.add_entity();
auto& text = textEntity.add<TextComponent>();

// Configure content
text.text = "Score: 0";

// Configure positioning
text.anchor = TextComponent::Anchor::TopRight;
text.position = glm::vec2(-20.0f, 20.0f);  // 20px from top-right corner
text.fontSize = 24.0f;

// Configure layout
text.alignment = TextComponent::Alignment::Right;
text.letterSpacing = 1.0f;

// Style with outline
text.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);        // White
text.outlineWidth = 0.15f;
text.outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);  // Black outline

// Add drop shadow
text.glowStrength = 0.4f;
text.glowColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.6f);

// Configure rendering
text.layer = 10;
text.visible = true;
```

---

## Common Use Cases

### Score Display (Top-Right)
```cpp
auto& score = entity.add<TextComponent>();
score.text = "Score: 1000";
score.anchor = TextComponent::Anchor::TopRight;
score.position = glm::vec2(-20, 20);
score.fontSize = 20.0f;
score.alignment = TextComponent::Alignment::Right;
score.color = glm::vec4(1, 1, 1, 1);
score.outlineWidth = 0.12f;
score.outlineColor = glm::vec4(0, 0, 0, 1);
score.layer = 10;
```

### Title Screen (Centered)
```cpp
auto& title = entity.add<TextComponent>();
title.text = "GAME TITLE";
title.anchor = TextComponent::Anchor::Center;
title.position = glm::vec2(0, -100);  // Slightly above center
title.fontSize = 72.0f;
title.alignment = TextComponent::Alignment::Center;
title.color = glm::vec4(1.0f, 0.84f, 0.0f, 1.0f);  // Gold
title.outlineWidth = 0.2f;
title.outlineColor = glm::vec4(0.2f, 0.1f, 0.0f, 1.0f);
title.glowStrength = 0.6f;
title.glowColor = glm::vec4(1.0f, 0.5f, 0.0f, 0.5f);  // Orange glow
title.layer = 50;
```

### FPS Counter (Top-Left)
```cpp
auto& fps = entity.add<TextComponent>();
fps.text = "FPS: 60";
fps.anchor = TextComponent::Anchor::TopLeft;
fps.position = glm::vec2(10, 10);
fps.fontSize = 14.0f;
fps.alignment = TextComponent::Alignment::Left;
fps.color = glm::vec4(0.0f, 1.0f, 0.0f, 0.8f);  // Semi-transparent green
fps.layer = 255;  // Always on top
```

### Tooltip (Bottom-Center)
```cpp
auto& tooltip = entity.add<TextComponent>();
tooltip.text = "Press E to interact";
tooltip.anchor = TextComponent::Anchor::BottomCenter;
tooltip.position = glm::vec2(0, -50);
tooltip.fontSize = 16.0f;
tooltip.alignment = TextComponent::Alignment::Center;
tooltip.color = glm::vec4(1, 1, 1, 1);
tooltip.outlineWidth = 0.1f;
tooltip.outlineColor = glm::vec4(0, 0, 0, 1);
tooltip.maxWidth = 300.0f;  // Wrap long text
tooltip.layer = 100;
```

### Health Bar Text
```cpp
auto& health = entity.add<TextComponent>();
health.text = "100 / 100";
health.anchor = TextComponent::Anchor::TopLeft;
health.position = glm::vec2(20, 50);
health.fontSize = 18.0f;
health.color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);  // Green (full health)
health.outlineWidth = 0.15f;
health.outlineColor = glm::vec4(0, 0, 0, 1);
health.layer = 10;

// Update color based on health percentage
if (healthPercent < 0.3f) {
    health.color = glm::vec4(1, 0, 0, 1);  // Red (low health)
}
```

### Multi-line Dialog
```cpp
auto& dialog = entity.add<TextComponent>();
dialog.text = "NPC: Hello traveler!\nHow can I help you today?";
dialog.anchor = TextComponent::Anchor::BottomLeft;
dialog.position = glm::vec2(20, -100);
dialog.fontSize = 16.0f;
dialog.alignment = TextComponent::Alignment::Left;
dialog.lineSpacing = 1.3f;
dialog.maxWidth = 400.0f;
dialog.color = glm::vec4(1, 1, 1, 1);
dialog.outlineWidth = 0.12f;
dialog.layer = 20;
```

---

## Anchor Point Reference

Visual guide showing where each anchor places the origin (0, 0):

```
Screen Dimensions: 1920×1080

TopLeft (0,0)      TopCenter (960,0)      TopRight (1920,0)
    ┌─────────────────┬─────────────────┐
    │                 │                 │
    │                 │                 │
    ├─────────────────┼─────────────────┤
    │                 │                 │
CenterLeft          Center          CenterRight
(0,540)          (960,540)         (1920,540)
    │                 │                 │
    ├─────────────────┼─────────────────┤
    │                 │                 │
    │                 │                 │
    └─────────────────┴─────────────────┘
BottomLeft         BottomCenter      BottomRight
(0,1080)           (960,1080)        (1920,1080)
```

**Example**: If screen is 1920×1080:
- `TopLeft` → origin at (0, 0)
- `Center` → origin at (960, 540)
- `BottomRight` → origin at (1920, 1080)

---

## Performance Considerations

### Text Updates
- Changing `text` content triggers re-layout (moderate cost)
- Changing visual properties (color, visible, rotation) is cheap
- Minimize per-frame text content changes

### Rendering Cost
- Each TextComponent is batched with others using the same font
- Layer sorting adds minor overhead
- Outline and glow effects add fragment shader cost (minimal with SDF)

### Optimization Tips
```cpp
// EXPENSIVE (every frame):
healthText.text = std::to_string(currentHealth);  // Avoid!

// BETTER (only when changed):
if (currentHealth != previousHealth) {
    healthText.text = std::to_string(currentHealth);
    previousHealth = currentHealth;
}

// BEST (pre-format strings):
static const std::array<std::string, 101> healthStrings = /* ... */;
healthText.text = healthStrings[currentHealth];
```

---

## Related Systems

- **RenderSystem** (`engine/src/Render/Render.cpp`): Processes TextComponent and submits to TextRenderer
- **TextRenderer** (`lib/Graphics/src/Rendering/TextRenderer.cpp`): CPU-side text layout and GPU submission
- **Font Importer** (`lib/resource/ext/include/importer/font.hpp`): Creates font atlas assets
- **Shaders**:
  - `bin/assets/shaders/text_sdf.vert`: Vertex shader for screen-space text
  - `bin/assets/shaders/text_sdf.frag`: Fragment shader with SDF rendering

---

## See Also

- [TextMeshComponent Documentation](TextMeshComponent.md) - For 3D world-space text
- [Billboard Modes Documentation](billboard_modes.md) - For world-space text orientation
- [CLAUDE.md](../CLAUDE.md) - Project architecture overview
