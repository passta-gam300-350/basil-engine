# World Text Billboard Modes

## Overview

Billboard modes control how world-space text orients itself relative to the camera. The text rendering system supports three billboard modes, each with different rotation behaviors optimized for specific use cases.

## Billboard Modes

### 1. Full Billboard

**"Text always faces the camera completely"**

#### Behavior
- Text rotates on **all axes** to directly face the camera
- Always perfectly readable from any viewing angle
- Acts like a sprite that's "glued" to face the viewer
- Both horizontal and vertical rotation applied

#### Implementation
```cpp
// Full billboard - text always faces camera (TextRenderer.cpp:632-638)
glm::vec3 toCamera = glm::normalize(worldText.cameraPosition - worldText.worldPosition);
billboardRight = glm::normalize(glm::cross(worldText.cameraUp, toCamera));
billboardUp = -glm::normalize(glm::cross(billboardRight, toCamera));
```

**Key Points:**
- `billboardRight` and `billboardUp` are recalculated every frame
- Orientation depends entirely on camera position
- Text normal always points toward camera

#### Visual Example
```
Camera looking down from above:
    📷 (camera above)
    |
    v
  [Text] ← Text tilts up to face camera

Camera at eye level:
📷 -----> [Text] ← Text stands upright

Camera looking up from below:
    ^
    |
    📷 (camera below)
  [Text] ← Text tilts down to face camera
```

#### Use Cases
- **Health bars** above characters
- **Damage numbers** in combat
- **Name tags** and player labels
- **Tooltips** and interactive prompts
- **UI elements** in 3D space
- Any text that must always be readable

---

### 2. Cylindrical Billboard

**"Text rotates horizontally but stays upright"**

#### Behavior
- Text only rotates around the **Y-axis** (vertical world axis)
- Always maintains vertical orientation (stays upright)
- Like a signpost or billboard that rotates on a pole
- Horizontal rotation only - no tilting

#### Implementation
```cpp
// Cylindrical billboard - rotate around Y-axis only (TextRenderer.cpp:640-653)
glm::vec3 toCamera = worldText.cameraPosition - worldText.worldPosition;
toCamera.y = 0.0f;  // Project onto XZ plane (remove vertical component)
toCamera = glm::normalize(toCamera);
billboardRight = glm::normalize(glm::cross(glm::vec3(0, 1, 0), toCamera));
billboardUp = glm::vec3(0, 1, 0);  // ALWAYS points up (hardcoded)
```

**Key Points:**
- `toCamera.y = 0.0f` removes vertical component (projects to XZ plane)
- `billboardUp` is always `(0, 1, 0)` - never changes
- Only `billboardRight` rotates horizontally
- Special handling when camera is directly above/below (uses camera forward)

#### Visual Example
```
Camera at eye level to the right:
    📷 -----> [Text] ← Text rotates to face camera, stays upright

Camera above looking down:
    📷 (camera above)
    |
    v
  [Text] ← Text still upright (you see top of letters)

Camera below looking up:
    ^
    |
    📷 (camera below)
  [Text] ← Text still upright (you see bottom of letters)
```

#### Use Cases
- **Tree/grass labels** in nature
- **Building signs** and storefronts
- **Street signs** and directional markers
- **Location markers** on terrain
- **Waypoint indicators**
- Any text that should feel "grounded" in the world

---

### 3. None (No Billboard)

**"Text uses entity's transform rotation (fixed orientation)"**

#### Behavior
- Text follows the entity's rotation matrix exactly
- No automatic rotation toward camera
- Behaves like regular 3D geometry
- Orientation determined by entity transform

#### Implementation
```cpp
// No billboard - use custom rotation from transform (TextRenderer.cpp:655-660)
billboardRight = glm::vec3(worldText.customRotation[0]);  // Entity's local X-axis
billboardUp = glm::vec3(worldText.customRotation[1]);     // Entity's local Y-axis
```

**Key Points:**
- Uses entity's transform matrix directly
- `customRotation` is the upper-left 3x3 of the world transform matrix
- Includes entity scale (if present in transform)
- No recalculation based on camera

#### Visual Example
```
Entity rotated 45° on Y-axis:
    📷 camera

   [Text]  ← Text stays at 45°, doesn't face camera
  /
 /

Entity rotated upside down:
        📷

 ┴xǝ┴  ← Text is upside down (follows entity rotation)
```

#### Use Cases
- **Text on walls/surfaces** (posters, graffiti)
- **Text part of 3D objects** (buttons on machines, control panels)
- **In-world readable signs** (road signs, billboards with fixed orientation)
- **Decals and surface text**
- When you want full artistic control over text orientation

---

## Comparison Table

| Feature | **Full** | **Cylindrical** | **None** |
|---------|----------|----------------|----------|
| **Horizontal Rotation** | ✅ Yes | ✅ Yes | ❌ No |
| **Vertical Rotation** | ✅ Yes | ❌ No (stays upright) | ❌ No |
| **Always Readable** | ✅ Yes, from any angle | ⚠️ Only from horizontal angles | ❌ No guarantee |
| **billboardUp** | Calculated per-frame | `(0, 1, 0)` hardcoded | From entity transform |
| **billboardRight** | Calculated per-frame | Calculated per-frame (XZ only) | From entity transform |
| **Performance** | Medium (2 cross products) | Low (1 cross product) | Lowest (no calculation) |
| **Feels Like** | 2D UI in 3D space | Physical sign in world | 3D geometry |

---

## Technical Details

### Coordinate System

The text rendering system uses:
- **Right-handed coordinate system**
- **Y-up** world orientation
- **Text layout** uses Y-down pixel space (converted to Y-up world space in shader)

### Vector Calculations

**Cross Product Convention:**
```cpp
billboardRight = cross(worldUp, toCamera)     // Up × Forward = Right
billboardUp = cross(billboardRight, toCamera) // Right × Forward = Up (then negated)
```

**Why Negate billboardUp in Full Mode?**

The `toCamera` vector points TOWARD the camera (backward from text's perspective). This causes:
```cpp
billboardUp = cross(billboardRight, backward)  // Results in DOWN vector
billboardUp = -cross(billboardRight, backward) // Negate to get UP vector
```

### Shader Application

All three modes feed into the same shader (`text_sdf_world.vert`):

```glsl
vec3 worldPos = instance.worldPosition
    + instance.billboardRight * (localPos.x * instance.size.x)
    + instance.billboardUp * (-localPos.y * instance.size.y);
```

The shader:
- Uses `billboardRight` for horizontal positioning (cursor advancement)
- Uses `billboardUp` for vertical positioning (baseline offset)
- Negates Y because quad uses Y-down convention but world is Y-up

---

## Decision Guide

### Choose **Full Billboard** when:
- ✅ Readability is critical
- ✅ Text is UI-like (health bars, tooltips, labels)
- ✅ You want screen-space feel in 3D world
- ✅ Player needs to read it from any angle
- ❌ Don't use if: Text should feel like part of the world

### Choose **Cylindrical Billboard** when:
- ✅ Text should feel "grounded" in the world
- ✅ Vertical orientation matters (shouldn't tilt)
- ✅ You want realistic sign/billboard effect
- ✅ Text represents physical objects (signs, markers)
- ❌ Don't use if: Camera frequently views from above/below

### Choose **None** when:
- ✅ Text is painted/attached to 3D surfaces
- ✅ Orientation is part of the design
- ✅ You want full control over rotation
- ✅ Text is decorative world geometry
- ❌ Don't use if: Readability from all angles is required

---

## Common Patterns

### Hybrid Approach
Many games combine billboard modes:
```
Player name tags:        Full Billboard (always readable)
Quest markers:           Cylindrical (grounded feel, points up)
Sign on wall:            None (fixed to wall orientation)
```

### Performance Considerations

**Rendering Cost (per text element per frame):**
- **None**: Cheapest (no calculations)
- **Cylindrical**: Low (1 normalize, 1 cross product)
- **Full**: Medium (2 normalizes, 2 cross products)

For hundreds of text elements, consider using:
- **None** for static/distant text
- **Cylindrical** for mid-range labels
- **Full** only for critical nearby UI

---

## Troubleshooting

### Text appears mirrored
**Cause:** Wrong cross product argument order
**Fix:** Ensure using `cross(up, forward)` not `cross(forward, up)`

### Text appears upside down (Full mode)
**Cause:** `toCamera` points toward camera, cross product needs negation
**Fix:** Negate billboardUp calculation: `billboardUp = -cross(billboardRight, toCamera)`

### Text stays flat on ground (Cylindrical)
**Cause:** billboardUp not set to world up
**Fix:** Ensure `billboardUp = vec3(0, 1, 0)`

### Cylindrical text flips when camera is overhead
**Cause:** XZ projection length becomes zero
**Fix:** Use camera forward as fallback (already implemented in line 644-648)

---

## Code References

- **Billboard mode enum**: `lib/Graphics/include/Rendering/TextRenderer.h`
- **CPU calculation**: `lib/Graphics/src/Rendering/TextRenderer.cpp:631-660`
- **Shader implementation**: `bin/assets/shaders/text_sdf_world.vert:54-64`
- **Transform setup**: `engine/src/Render/Render.cpp:567-569`

---

## History

This documentation reflects the final implementation after fixing several billboard orientation bugs:
- Fixed Full mode cross product order (was mirrored)
- Fixed Full mode billboardUp negation (was upside down)
- Fixed Cylindrical mode cross product order (was mirrored)
- Fixed shader quad anchoring (character spacing issues)

For implementation details, see commit history or `CLAUDE.md`.
