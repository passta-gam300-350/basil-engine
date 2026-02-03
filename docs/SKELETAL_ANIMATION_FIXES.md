# Skeletal Animation Bug Fixes

This document records all fixes made to the Graphics library (`lib/Graphics/`) and related files to enable skeletal animation rendering. These changes are minimal and targeted — they fix pre-existing bugs rather than restructuring the codebase.

---

## Summary of Changes

| # | File | Severity | What Changed |
|---|------|----------|-------------|
| 1 | `lib/Graphics/src/Buffer/VertexArray.cpp` | **CRITICAL** | Added `GL_INT` to `GetSizeOfType()` + use `glVertexAttribIPointer` for integer attributes |
| 2 | `lib/Graphics/src/Rendering/InstancedRenderer.cpp` | HIGH | `Clear()` now resets tracking state; `BuildDynamicInstanceData()` clears skinned list; added `RenderSkinnedMeshes()` |
| 3 | `lib/Graphics/src/Pipeline/MainRenderingPass.cpp` | HIGH | Added `ClearCommands()` after `ExecuteCommands()` before skinned mesh rendering |
| 4 | `lib/Graphics/include/Rendering/PBRLightingRenderer.h` | LOW | Added public `ResetFrameCache()` method |
| 5 | `lib/Graphics/include/Rendering/InstancedRenderer.h` | LOW | Added `m_SkinnedInstanceSSBO` member |
| 6 | `bin/assets/shaders/main_pbr.vert` | **CRITICAL** | Fixed `aPos` -> `skinnedPos` in world position calculation |

---

## Fix 1: VertexArray — Integer Attribute Setup (CRITICAL)

**File:** `lib/Graphics/src/Buffer/VertexArray.cpp`

**Two sub-bugs fixed:**

### 1a. Missing `GL_INT` in `GetSizeOfType()`

```cpp
// BEFORE (BUG): GL_INT not handled, returns 0
switch (type) {
    case GL_FLOAT: return sizeof(GLfloat);
    case GL_UNSIGNED_INT: return sizeof(GLuint);
    case GL_UNSIGNED_BYTE: return sizeof(GLubyte);
    default: return 0;  // GL_INT falls here -> returns 0!
}

// AFTER (FIX): GL_INT added
switch (type) {
    case GL_FLOAT: return sizeof(GLfloat);
    case GL_INT: return sizeof(GLint);       // <-- ADDED
    case GL_UNSIGNED_INT: return sizeof(GLuint);
    case GL_UNSIGNED_BYTE: return sizeof(GLubyte);
    default: return 0;
}
```

**Impact:** The `Vertex` struct layout is `{Position(12), Normal(12), TexCoords(8), Tangent(12), Bitangent(12), BoneIDs(16), Weights(16)}` = 88 bytes. Without `GL_INT` returning a proper size, `Push<int>(4)` added 0 to the stride calculation, making the computed stride **72 instead of 88**. This also made the Weights attribute (location 6) point to the **same offset** as BoneIDs (location 5) — offset 56 instead of 72. Every mesh vertex beyond index 0 had misaligned attribute data.

**Risk:** This fix affects ALL meshes (not just skinned ones) because all meshes use the same `Vertex` struct with bone fields. However, the fix is **strictly correct** — the previous stride was wrong. Non-skinned meshes that appeared to work were actually rendering with slightly garbled vertex data for vertices beyond index 0.

### 1b. `glVertexAttribPointer` -> `glVertexAttribIPointer` for integer types

```cpp
// BEFORE (BUG): All attributes use glVertexAttribPointer
glVertexAttribPointer(i, count, type, normalized, stride, offset);

// AFTER (FIX): Integer types use glVertexAttribIPointer
if (element.type == GL_INT || element.type == GL_UNSIGNED_INT || ...)
{
    glVertexAttribIPointer(i, count, type, stride, offset);
}
else
{
    glVertexAttribPointer(i, count, type, normalized, stride, offset);
}
```

**Impact:** The vertex shader declares `layout(location = 5) in ivec4 aBoneIDs`. OpenGL requires `glVertexAttribIPointer` for integer shader inputs. Using `glVertexAttribPointer` converts the data through the float pipeline, producing garbage values when read as `ivec4`. This caused the skinning matrix to index into random SSBO locations, producing NaN/degenerate transforms.

**Risk:** Low. The only integer attribute in the current `Vertex` struct is `m_BoneIDs[4]` (via `Push<int>(4)`). No code uses `Push<uint8_t>()` or `Push<uint32_t>()`, so no other attribute types are affected.

### How to Revert

If anything breaks, you can revert just this file:
```bash
git checkout origin/m4_zen -- lib/Graphics/src/Buffer/VertexArray.cpp
```
Or revert only the `glVertexAttribIPointer` change while keeping the `GL_INT` size fix (which is unconditionally correct).

---

## Fix 2: InstancedRenderer — Tracking State Reset & Skinned Mesh Support

**File:** `lib/Graphics/src/Rendering/InstancedRenderer.cpp`

### 2a. `Clear()` now resets all tracking state

```cpp
// BEFORE (BUG): Clear() only wiped mesh data
void InstancedRenderer::Clear() {
    m_MeshInstances.clear();
    m_InstanceSSBOs.clear();
    m_TotalInstances = 0;
    m_BatchActive = false;
    // Tracking state (m_LastRenderableCount, m_LastObjectIDs, etc.) NOT reset!
}

// AFTER (FIX): Clear() also resets tracking state
void InstancedRenderer::Clear() {
    m_MeshInstances.clear();
    m_InstanceSSBOs.clear();
    m_TotalInstances = 0;
    m_BatchActive = false;
    m_SkinnedRenderables.clear();       // <-- ADDED
    m_LastRenderableCount = 0;          // <-- ADDED
    m_LastObjectIDs.clear();            // <-- ADDED
    m_LastTransformHashes.clear();      // <-- ADDED
    m_LastPropertyBlockHashes.clear();  // <-- ADDED
    m_LastMaterialPointers.clear();     // <-- ADDED
    m_LastMeshPointers.clear();         // <-- ADDED
}
```

**Impact:** When `ClearInstanceCache()` called `Clear()` every frame (needed for animated objects), the mesh data was wiped but tracking state remained stale. On the next frame, `HasRenderablesChanged()` compared against the old tracking data and returned `false`, so `BuildDynamicInstanceData()` never ran, leaving instances empty. Nothing rendered.

**Risk:** Low. This only adds more thorough cleanup to `Clear()`. It does not change any rendering logic.

### 2b. `BuildDynamicInstanceData()` clears `m_SkinnedRenderables` at start

Previously, skinned renderables accumulated across frames because the list was never cleared at the right point. Now `BuildDynamicInstanceData()` clears it before rebuilding.

**Risk:** None — this is purely a leak fix.

### 2c. `RenderSkinnedMeshes()` — New method for skinned mesh rendering

This is a new method that renders skinned meshes individually (not batched) via the deferred command system. Each skinned mesh gets its own submit-execute cycle with:
- Bone matrix SSBO upload (binding point 2)
- Instance data SSBO upload (binding point 0)
- Full lighting/shadow/texture setup
- DrawElementsInstanced with instanceCount=1

**Risk:** This is new code, not a modification of existing rendering paths. It only executes when `m_SkinnedRenderables` is non-empty (i.e., when `RenderableData.isSkinned == true`). If no skinned renderables exist, the method returns immediately.

---

## Fix 3: MainRenderingPass — Command Buffer Isolation for Skinned Meshes

**File:** `lib/Graphics/src/Pipeline/MainRenderingPass.cpp`

```cpp
// BEFORE (BUG): No ClearCommands(), no skinned mesh call
ExecuteCommands();
// ... store frame data, End()

// AFTER (FIX): Clear buffer, then render skinned meshes
ExecuteCommands();
ClearCommands();  // <-- ADDED: prevent re-execution of main pass commands
context.instancedRenderer.RenderSkinnedMeshes(*this, context.frameData);  // <-- ADDED
// ... store frame data, End()
```

**Impact:** Without `ClearCommands()`, calling `RenderSkinnedMeshes()` (which does its own `ExecuteCommands()`) would re-execute the ENTIRE main pass (clear screen, skybox, all opaque draws). The `ClearCommands()` ensures skinned meshes get a clean command buffer.

**Risk:** Low. `ClearCommands()` only empties the command queue — it does not affect any GPU state. The main pass commands have already been executed at this point.

---

## Fix 4: PBRLightingRenderer — ResetFrameCache()

**File:** `lib/Graphics/include/Rendering/PBRLightingRenderer.h`

```cpp
// ADDED: Public method to reset per-frame caches
void ResetFrameCache() { m_LastLightingShader.reset(); m_LastShadowShader.reset(); }
```

**Impact:** The PBR lighting system caches which shader has had lighting uniforms uploaded to avoid redundant uploads. After `ExecuteCommands()` + `CleanupGPUState()` unbinds the shader program, the skinned mesh needs lighting re-submitted. `ResetFrameCache()` forces this re-submission.

**Risk:** None. This only invalidates a performance cache, causing one extra set of uniform uploads per frame for skinned meshes. It does not change what data is uploaded.

---

## Fix 5: InstancedRenderer.h — Skinned Instance SSBO

**File:** `lib/Graphics/include/Rendering/InstancedRenderer.h`

```cpp
// ADDED: Instance SSBO for skinned meshes (single-element, reused per skinned draw)
std::unique_ptr<ShaderStorageBuffer> m_SkinnedInstanceSSBO;
```

**Impact:** Skinned meshes need their own instance data SSBO (binding point 0) separate from the batched instancing SSBOs. This member is created on first use and reused across frames.

**Risk:** None. This is a new member with no effect on existing code paths.

---

## Fix 6: Vertex Shader — Skinned Position Output (CRITICAL)

**File:** `bin/assets/shaders/main_pbr.vert`

```glsl
// BEFORE (BUG): Used original aPos, completely ignoring skinning result
vec4 worldPos = model * vec4(aPos, 1.0);
gl_Position = u_Projection * u_View * worldPos;

// AFTER (FIX): Uses skinnedPos (which equals aPos when skinning is disabled)
vec4 worldPos = model * vec4(skinnedPos, 1.0);
gl_Position = u_Projection * u_View * worldPos;
```

**Impact:** The skinning block computed `skinnedPos` from bone matrices but the result was never used — the world position always used the original `aPos`. Skinned meshes rendered in bind pose regardless of animation.

**Risk:** None for non-skinned meshes. When `u_EnableSkinning` is `false`, `skinnedPos` is initialized to `aPos` before the skinning block, so the behavior is identical to the old code.

---

## Files NOT Modified (Safe)

The following graphics library files were **not** changed:
- `RenderCommandBuffer.cpp` — Command execution logic untouched
- `ShaderStorageBuffer.cpp` — SSBO creation/upload untouched
- `SceneRenderer.cpp` — Scene orchestration untouched
- `PBRLightingRenderer.cpp` — Lighting logic untouched (only header changed)
- All shadow passes, bloom, HDR, tone mapping — untouched
- All other render passes — untouched
- Fragment shader (`main_pbr.frag`) — untouched

---

## Reverting All Changes

To revert all graphics library changes back to the branch baseline:

```bash
# Revert individual files
git checkout origin/m4_zen -- lib/Graphics/src/Buffer/VertexArray.cpp
git checkout origin/m4_zen -- lib/Graphics/src/Rendering/InstancedRenderer.cpp
git checkout origin/m4_zen -- lib/Graphics/src/Pipeline/MainRenderingPass.cpp
git checkout origin/m4_zen -- lib/Graphics/include/Rendering/PBRLightingRenderer.h
git checkout origin/m4_zen -- lib/Graphics/include/Rendering/InstancedRenderer.h
git checkout origin/m4_zen -- bin/assets/shaders/main_pbr.vert
```

**Note:** Fix 1 (VertexArray `GL_INT` + `glVertexAttribIPointer`) and Fix 6 (shader `skinnedPos`) are genuine pre-existing bugs that affect correctness regardless of skeletal animation. Consider keeping them even if other changes are reverted.
