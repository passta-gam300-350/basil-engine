# Skeletal Animation System - Implementation Summary

## Overview

This document describes the skeletal animation system implementation completed over 2 days. The system supports multi-bone skeletal animation with GPU skinning, animation blending, and backward compatibility with the existing simple (single-bone) animation system.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         ENGINE SIDE                                 │
│                                                                     │
│  AnimationComponent         SkeletonComponent                       │
│  ├── animator* instance     ├── skeleton (bone hierarchy)           │
│  ├── isSkeletalAnim         ├── finalBoneMatrices (GPU output)      │
│  ├── blendState             └── globalBoneTransforms                │
│  └── playback state                                                 │
│           │                          ▲                              │
│           ▼                          │                              │
│  AnimationSystem::FixedUpdate()                                     │
│  ├── Skeletal path: animator->updateAnimation() → writes matrices   │
│  └── Simple path: single boneChannel → updates TransformComponent   │
│                                                                     │
│  RenderSystem::Update() (Render.cpp)                                │
│  └── Checks for SkeletonComponent → sets RenderableData bone fields │
│                                                                     │
├─────────────────────────────────────────────────────────────────────┤
│                        GRAPHICS SIDE                                │
│                                                                     │
│  RenderableData                                                     │
│  ├── boneMatrices (pointer to SkeletonComponent data)               │
│  ├── boneCount                                                      │
│  └── isSkinned                                                      │
│           │                                                         │
│           ▼                                                         │
│  InstancedRenderer                                                  │
│  ├── Non-skinned → batched instanced rendering (existing)           │
│  └── Skinned → individual rendering with bone SSBO (new)            │
│                                                                     │
│  main_pbr.vert (Shader)                                             │
│  ├── u_EnableSkinning = false → standard rendering                  │
│  └── u_EnableSkinning = true → apply bone matrix skinning           │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Day 1: CPU-Side Implementation

### 1. Created SkeletonComponent
**File:** `engine/include/Component/SkeletonComponent.hpp`

Holds the skeleton data and output bone matrices:
- `skeleton skeletonData` - bone hierarchy (names, parent indices, inverse bind matrices)
- `std::vector<glm::mat4> finalBoneMatrices` - output for GPU skinning
- `std::vector<glm::mat4> globalBoneTransforms` - world-space bone poses
- Helper methods: `InitializeMatrices()`, `GetBoneCount()`, `FindBoneIndex()`

### 2. Updated AnimationComponent
**File:** `engine/include/Component/AnimationComponent.hpp`

Added fields for skeletal animation:
- `animator* animatorInstance` - pointer to animator (null for simple animation)
- `blendState blend` - for smooth animation transitions
- `bool isSkeletalAnim` - flag to switch between skeletal/simple mode

### 3. Refactored AnimationSystem
**File:** `engine/src/System/AnimationSystem.cpp`

Split into two paths:

**Skeletal Animation Path:**
- Queries entities with `AnimationComponent` + `SkeletonComponent`
- Uses `animator::updateAnimation()` to process all bone channels
- Copies `finalBoneMatrices` from animator to SkeletonComponent

**Simple Animation Path (backward compatible):**
- Queries entities with `AnimationComponent` + `TransformComponent`
- Existing single-bone behavior unchanged
- Skips entities where `isSkeletalAnim == true`

### 4. Added InitializeSkeletalAnimation Helper
**File:** `engine/src/System/AnimationSystem.cpp`

Helper function for teammates to set up skeletal animation:
```cpp
void InitializeSkeletalAnimation(
    AnimationComponent& animComp,
    SkeletonComponent& skelComp,
    const skeleton& skeletonData,
    animationContainer* animation
);
```

Sets up skeleton data, creates animator with correct bone count, and enables skeletal mode.

---

## Day 2: GPU-Side Implementation

### 5. Updated Vertex Shader
**File:** `bin/assets/shaders/main_pbr.vert`

Added:
- Bone vertex attributes: `aBoneIDs` (ivec4, location 5), `aWeights` (vec4, location 6)
- Bone matrix SSBO: binding point 2
- Skinning uniforms: `u_EnableSkinning`, `u_BoneOffset`
- Skinning calculation: matrix palette skinning with 4 bone influences per vertex
- Applied to position, normal, tangent, and bitangent

### 6. Updated RenderableData
**File:** `lib/Graphics/include/Utility/RenderData.h`

Added fields:
- `const glm::mat4* boneMatrices` - pointer to bone matrix array
- `uint32_t boneCount` - number of bones
- `bool isSkinned` - flag for skinned mesh detection

### 7. Updated InstancedRenderer
**Files:** `lib/Graphics/include/Rendering/InstancedRenderer.h`, `lib/Graphics/src/Rendering/InstancedRenderer.cpp`

Added:
- `m_BoneMatrixSSBO` - SSBO for bone matrices (binding 2, max 4096 bones)
- `m_SkinnedRenderables` - stores skinned meshes separately from batched rendering
- `UploadBoneMatrices()` - uploads bone matrices to GPU
- `RenderSkinnedMeshes()` - renders each skinned mesh individually with its bone data
- `u_EnableSkinning = false` set explicitly for non-skinned batched meshes

### 8. Engine-Side Wiring
**File:** `engine/src/Render/Render.cpp`

In `RenderSystem::Update()`, checks if entity has `SkeletonComponent` and populates bone data in RenderableData before submitting to renderer.

---

## Key Design Decisions

### Separate SkeletonComponent vs AnimationComponent
- **SkeletonComponent**: Static bone hierarchy + runtime matrices (output)
- **AnimationComponent**: Dynamic animation state (input)
- One skeleton can have many animations; entities can have skeleton without animation (ragdoll)

### Animator stored as pointer in AnimationComponent
- `animator*` is null for simple animation entities (saves memory)
- Created via `InitializeSkeletalAnimation()` when skeleton is set up
- Manages its own playback time and blending state

### Skinned meshes rendered individually (not batched)
- Each skinned mesh has unique bone matrices
- Cannot batch like static meshes
- Rendered after batched meshes in `RenderToPass()`

### Bone matrices passed as pointer in RenderableData
- Avoids copying (128 bones x 64 bytes = 8KB per entity)
- Points directly to SkeletonComponent data
- Valid for the frame's lifetime

---

## Files Modified/Created

| File | Action |
|------|--------|
| `engine/include/Component/SkeletonComponent.hpp` | **Created** |
| `engine/include/Component/AnimationComponent.hpp` | Modified |
| `engine/include/System/AnimationSystem.hpp` | Modified |
| `engine/src/System/AnimationSystem.cpp` | Modified |
| `engine/src/Render/Render.cpp` | Modified |
| `lib/Graphics/include/Utility/RenderData.h` | Modified |
| `lib/Graphics/include/Rendering/InstancedRenderer.h` | Modified |
| `lib/Graphics/src/Rendering/InstancedRenderer.cpp` | Modified |
| `bin/assets/shaders/main_pbr.vert` | Modified |

---

## How to Use (For Teammates)

### Setting up a skeletal animation entity:

```cpp
// 1. Create entity with required components
auto entity = world.create();
entity.emplace<MeshRendererComponent>();      // Skinned mesh
entity.emplace<TransformComponent>();
entity.emplace<AnimationComponent>();
entity.emplace<SkeletonComponent>();

// 2. Get component references
auto& animComp = entity.get<AnimationComponent>();
auto& skelComp = entity.get<SkeletonComponent>();

// 3. Initialize (skeleton data from model loader, animation from resource system)
InitializeSkeletalAnimation(animComp, skelComp, loadedSkeleton, loadedAnimation);

// Done! AnimationSystem will update bone matrices each frame.
// RenderSystem will pass them to GPU for skinning.
```

### Adding more animations:

```cpp
// Add animation to animator
animComp.animatorInstance->addAnimation("Walk", walkAnimation);
animComp.animatorInstance->addAnimation("Run", runAnimation);

// Play animation (auto-blends if another is playing)
animComp.animatorInstance->playAnimation("Run");
```

---

## Known Limitations

1. **Shadow shaders** do not support skinning yet - shadows will use bind pose
2. **No editor UI** for skeleton/animation inspection
3. **Skinned meshes not batched** - each draws individually
4. **Memory cleanup** - `animator*` needs manual deletion (consider `unique_ptr`)
5. **Bone count limit** - 4096 total bones across all skinned meshes per frame

---

## Future Work

- Shadow shader skinning
- Bone debug visualization (draw skeleton as lines)
- Animation state machine
- Animation layers and additive blending
- Inverse kinematics (IK)
- Root motion extraction
- Animation events (callbacks at specific frames)
- Editor integration (reflection registration, inspector UI)
