# Skeletal Animation System - Implementation TODO

## Current State

**What's Already Implemented:**
- ✅ Animation data structures (`keyFramePosition`, `keyFrameRotation`, `keyFrameScale`)
- ✅ Bone channel with keyframe interpolation (`boneChannel`)
- ✅ Animation container structure (`animationContainer`)
- ✅ Skeleton structures (`oneSkeletonBone`, `skeleton`)
- ✅ `animator` class with blending support
- ✅ Vertex structure with `m_BoneIDs[4]` and `m_Weights[4]`
- ✅ Basic `AnimationComponent` in ECS
- ✅ Animation blending infrastructure (`blendState`)

**Day 1 Progress (CPU-side complete):**
- ✅ SkeletonComponent created
- ✅ AnimationComponent updated with `animator*`, `blendState`, `isSkeletalAnim`
- ✅ AnimationSystem refactored with two paths (skeletal + simple)
- ✅ `InitializeSkeletalAnimation()` helper function added

**Remaining Work:**
- GPU skinning (shaders)
- Rendering integration (upload bone matrices)
- Testing with actual skinned mesh

---

## Phase 1: ECS Component Setup ✅ COMPLETE

### 1.1 Create SkeletonComponent ✅
- [x] Create `engine/include/Component/SkeletonComponent.hpp`
- [x] Add `skeleton skeletonData` for bone hierarchy
- [x] Add `std::vector<glm::mat4> finalBoneMatrices` for GPU skinning
- [x] Add `std::vector<glm::mat4> globalBoneTransforms` for world-space poses
- [x] Add `InitializeMatrices()`, `GetBoneCount()`, `FindBoneIndex()` helpers
- [ ] Register component with ECS reflection system (optional, for editor)
- [ ] Add to `Component.def` for serialization (optional, for editor)

### 1.2 Refactor AnimationComponent ✅
- [x] Add `animator* animatorInstance` for skeletal animation
- [x] Add `blendState blend` for animation transitions
- [x] Add `bool isSkeletalAnim` flag to switch between skeletal/simple modes
- [ ] Update reflection bindings (optional, for editor)

### 1.3 Create SkinnedMeshComponent (Optional - Skipped for now)
- [ ] Flag to indicate mesh requires skinning
- [ ] Reference to skeleton component entity (if separate)
- [ ] Bone matrix buffer handle for GPU

---

## Phase 2: Skeleton Hierarchy Processing ✅ COMPLETE (via animator class)

### 2.1 Skeleton Data Loading
- [x] `skeleton` structure exists in Animation.h
- [x] `skeletonHelper::validateHierarchy()` validates bone hierarchy
- [x] `skeletonHelper::reorderHierarchy()` sorts bones parent-first
- [x] `skeletonHelper::hasCircularDependency()` checks for cycles
- [x] Inverse bind matrices stored in `oneSkeletonBone::inverseBind`

### 2.2 Implement Hierarchy Traversal ✅
- [x] `animator::calculateFinalBoneMatrices()` processes bones in order
- [x] Local transforms calculated from bone channels
- [x] Global transform: `globalTransforms[i] = globalTransforms[parent] * localTransform`
- [x] Root bones (parentIndex == -1) handled correctly

### 2.3 Apply Inverse Bind Matrices ✅
- [x] Final matrix: `finalBoneMatrices[i] = globalTransforms[i] * inverseBind[i]`
- [x] Results stored in SkeletonComponent via AnimationSystem

---

## Phase 3: AnimationSystem Refactor ✅ COMPLETE

### 3.1 Update System Query ✅
- [x] Query entities with `AnimationComponent` + `SkeletonComponent` (skeletal path)
- [x] Query entities with `AnimationComponent` + `TransformComponent` (simple path)
- [x] Added `SkeletonComponent` to system WriteSet

### 3.2 Implement Full Skeleton Update ✅
- [x] Uses `animator::updateAnimation()` which handles all bone channels
- [x] `animator::updateSingleAnimation()` processes all channels
- [x] Calls `calculateFinalBoneMatrices()` to compute final matrices
- [x] Results copied to `SkeletonComponent::finalBoneMatrices`

### 3.3 Animation Playback Controls ✅ (via animator class)
- [x] `animator::play()`, `pause()`, `stop()`
- [x] `animator::setLoop()` for loop vs play-once
- [x] `animator::setPlayBackSpeed()` for playback speed
- [x] `animator::isAnimationFinished()` handles animation end

### 3.4 Animation Blending ✅ (via animator class)
- [x] `blendState` infrastructure exists and works
- [x] `animator::updateBlendedAnimation()` samples both animations
- [x] `blendingHelper::blendTransforms()` interpolates with slerp for rotations
- [x] Crossfade transitions supported via `animator::playAnimation()`

### 3.5 Helper Function ✅
- [x] `InitializeSkeletalAnimation()` - sets up entity for skeletal animation

---

## Phase 4: GPU Skinning (Shaders)

### 4.1 Update Vertex Layout
- [ ] Add bone ID attribute (ivec4) at location 5
- [ ] Add bone weight attribute (vec4) at location 6
- [ ] Update `VertexArray` setup in Mesh class
- [ ] Ensure Assimp loader populates bone data in vertices

### 4.2 Create Bone Matrix Buffer
- [ ] Create SSBO for bone matrices (max ~256 bones per skeleton)
- [ ] Bind to appropriate binding point (e.g., binding = 3)
- [ ] Upload matrices from SkeletonComponent each frame

### 4.3 Update main_pbr.vert
- [ ] Add bone matrix SSBO:
  ```glsl
  layout(std430, binding = 3) readonly buffer BoneMatrices {
      mat4 u_BoneMatrices[];
  };
  ```
- [ ] Add skinning uniform flag: `uniform bool u_IsSkinned;`
- [ ] Implement matrix palette skinning:
  ```glsl
  mat4 skinMatrix =
      u_BoneMatrices[a_BoneIDs.x] * a_Weights.x +
      u_BoneMatrices[a_BoneIDs.y] * a_Weights.y +
      u_BoneMatrices[a_BoneIDs.z] * a_Weights.z +
      u_BoneMatrices[a_BoneIDs.w] * a_Weights.w;
  ```
- [ ] Apply to position: `vec4 skinnedPos = skinMatrix * vec4(aPos, 1.0);`
- [ ] Apply to normal: `vec3 skinnedNormal = mat3(skinMatrix) * aNormal;`
- [ ] Apply to tangent/bitangent for normal mapping

### 4.4 Update Shadow Shaders
- [ ] Add same skinning logic to `shadow_depth.vert`
- [ ] Add to `shadow_depth_instanced.vert`
- [ ] Add to `point_shadow_instanced.vert`

---

## Phase 5: Rendering Integration

### 5.1 Update RenderData Structure
- [ ] Add skinning flag to render data
- [ ] Add bone matrix buffer reference
- [ ] Add bone count

### 5.2 Update MainRenderingPass
- [ ] Check if mesh is skinned before rendering
- [ ] Bind bone matrix SSBO for skinned meshes
- [ ] Set `u_IsSkinned` uniform

### 5.3 Update InstancedRenderer
- [ ] Handle skinned mesh instances
- [ ] Each skinned instance needs its own bone matrices
- [ ] Consider per-instance bone matrix offset in SSBO

### 5.4 Update Shadow Passes
- [ ] Apply skinning in shadow map generation
- [ ] Ensure shadows match deformed mesh

---

## Phase 6: Debug & Visualization

### 6.1 Bone Debug Rendering
- [ ] Draw skeleton as lines (bone to parent)
- [ ] Show bone axes (local coordinate system)
- [ ] Visualize bone names in editor

### 6.2 Animation Preview
- [ ] Play/pause controls in editor
- [ ] Scrub timeline
- [ ] Show current frame/time

---

## Phase 7: Advanced Features (Optional)

### 7.1 Animation State Machine
- [ ] Define named states (Idle, Walk, Run, Jump)
- [ ] Define transitions with conditions
- [ ] Automatic blend on transition
- [ ] Support for trigger parameters

### 7.2 Animation Layers
- [ ] Base layer (full body)
- [ ] Additive layers (facial, upper body override)
- [ ] Layer weights and masks

### 7.3 Inverse Kinematics (IK)
- [ ] Foot IK for ground contact
- [ ] Hand IK for object interaction
- [ ] Look-at IK for head tracking

### 7.4 Root Motion
- [ ] Extract root bone translation
- [ ] Apply to entity transform
- [ ] Option to enable/disable per animation

### 7.5 Animation Events
- [ ] Trigger callbacks at specific frames
- [ ] Footstep sounds, particle effects, etc.

---

## File Locations Reference

| Component | File Path |
|-----------|-----------|
| Animation Data Structures | `lib/Graphics/include/Animation/Animation.h` |
| Animation Implementation | `lib/Graphics/src/Animation/Animation.cpp` |
| **SkeletonComponent** | `engine/include/Component/SkeletonComponent.hpp` ✅ NEW |
| AnimationComponent | `engine/include/Component/AnimationComponent.hpp` |
| AnimationSystem Header | `engine/include/System/AnimationSystem.hpp` |
| AnimationSystem Implementation | `engine/src/System/AnimationSystem.cpp` |
| Mesh Vertex Structure | `lib/Graphics/include/Resources/Mesh.h` |
| Model Loading | `lib/Graphics/src/Resources/Model.cpp` |
| Main PBR Shader | `bin/assets/shaders/main_pbr.vert` |
| Shadow Shader | `bin/assets/shaders/shadow_depth.vert` |
| Rig Resource | `lib/resource/core/include/native/rig.h` |
| Component Registry | `engine/include/Component/Component.def` |

---

## Implementation Order

1. ✅ **Phase 1** - ECS components (foundation) - DONE
2. ✅ **Phase 2** - Skeleton hierarchy (CPU-side logic) - DONE (via animator)
3. ✅ **Phase 3** - AnimationSystem refactor - DONE
4. ⬜ **Phase 4** - GPU skinning (see mesh deform) - **NEXT**
5. ⬜ **Phase 5** - Rendering integration
6. ⬜ **Phase 6** - Debug visualization (verify correctness)
7. ⬜ **Phase 7** - Advanced features as needed

---

## Testing Checklist

- [ ] Single bone animation still works (backwards compatibility)
- [ ] Multi-bone skeleton loads correctly
- [ ] Hierarchy traversal produces correct global transforms
- [ ] Inverse bind matrices produce correct skinning
- [ ] GPU skinning matches CPU reference
- [ ] Shadows render correctly for skinned meshes
- [ ] Animation blending transitions smoothly
- [ ] Performance acceptable (target: 100+ animated characters)

---

## Progress Summary

| Day | Focus | Status |
|-----|-------|--------|
| Day 1 | CPU-side (Components + AnimationSystem) | ✅ Complete |
| Day 2 | GPU-side (Shaders + Rendering) | ⬜ Pending |

---

*Last Updated: 2026-01-24*
