# Scene Graph System - Usage Guide

## Overview

The scene graph system provides Unity-style hierarchical parent-child relationships between entities with automatic transform propagation. When you transform a parent entity, all child entities are automatically transformed relative to their parent.

## Key Components

### 1. TransformComponent
- **Local Transform**: Position, rotation, and scale relative to parent
- **World Transform**: Absolute position in world space
- **Automatic Propagation**: Changes propagate through the hierarchy

### 2. RelationshipComponent
- Tracks parent-child relationships
- Automatically added to all game objects
- Manages bidirectional links

### 3. TransformHierarchySystem
- Updates transforms every frame
- Propagates parent transforms to children
- Runs at 60 FPS by default

## Basic Usage

### Creating a Parent-Child Relationship

```cpp
#include "Scene/SceneGraph.hpp"
#include "Manager/ObjectManager.hpp"

// Create entities
ecs::entity parent = ObjectManager::GetInstance().CreateGameObject();
ecs::entity child = ObjectManager::GetInstance().CreateGameObject();

// Set up parent-child relationship
SceneGraph::SetParent(child, parent);

// Or alternatively
SceneGraph::AddChild(parent, child);
```

### Transforming Entities

```cpp
// Get the transform component
auto& parentTransform = parent.get<TransformComponent>();

// Modify local transform (relative to parent)
parentTransform.localPosition = glm::vec3(10.0f, 0.0f, 0.0f);
parentTransform.localScale = glm::vec3(2.0f, 2.0f, 2.0f);
parentTransform.setRotationFromEuler(glm::vec3(0.0f, glm::radians(45.0f), 0.0f));

// Mark as dirty to trigger recalculation
SceneGraph::MarkTransformDirty(parent);

// The child's world transform will automatically update next frame!
```

### Maintaining World Position When Reparenting

```cpp
// Reparent but keep the child at its current world position
SceneGraph::SetParent(child, newParent, true); // keepWorldTransform = true

// This adjusts the child's local transform so its world position stays the same
```

### Removing Parent (Making Root Entity)

```cpp
// Remove parent
SceneGraph::RemoveParent(child);

// Or with keeping world transform
SceneGraph::RemoveParent(child, true);
```

## Advanced Usage

### Getting Hierarchy Information

```cpp
// Check if entity has a parent
if (SceneGraph::HasParent(entity))
{
    ecs::entity parent = SceneGraph::GetParent(entity);
    // Do something with parent
}

// Get all children
std::vector<ecs::entity> children = SceneGraph::GetChildren(parent);
for (const auto& child : children)
{
    // Process each child
}
```

### Accessing Transform Matrices

```cpp
// Get world transform
glm::mat4 worldMatrix = SceneGraph::GetWorldTransform(entity);

// Get local transform
glm::mat4 localMatrix = SceneGraph::GetLocalTransform(entity);

// Or access directly from component
auto& transform = entity.get<TransformComponent>();
glm::mat4 world = transform.worldMatrix;
glm::mat4 local = transform.getLocalMatrix();
```

### Using Quaternions vs Euler Angles

```cpp
auto& transform = entity.get<TransformComponent>();

// Set rotation using euler angles (in radians)
transform.setRotationFromEuler(glm::vec3(pitch, yaw, roll));

// Get rotation as euler angles (in radians)
glm::vec3 euler = transform.getEulerRotation();

// Or use quaternions directly
transform.localRotation = glm::quat(glm::vec3(pitch, yaw, roll));
```

### Force Recalculation

```cpp
// Mark single entity dirty
SceneGraph::MarkTransformDirty(entity);

// Mark entire hierarchy dirty (entity and all descendants)
SceneGraph::MarkHierarchyDirty(rootEntity);
```

## Example: Building a Solar System

```cpp
// Create sun (root)
ecs::entity sun = ObjectManager::GetInstance().CreateGameObject();
auto& sunTransform = sun.get<TransformComponent>();
sunTransform.localScale = glm::vec3(3.0f);

// Create Earth (child of sun)
ecs::entity earth = ObjectManager::GetInstance().CreateGameObject();
SceneGraph::SetParent(earth, sun);
auto& earthTransform = earth.get<TransformComponent>();
earthTransform.localPosition = glm::vec3(10.0f, 0.0f, 0.0f);
earthTransform.localScale = glm::vec3(1.0f);

// Create Moon (child of Earth)
ecs::entity moon = ObjectManager::GetInstance().CreateGameObject();
SceneGraph::SetParent(moon, earth);
auto& moonTransform = moon.get<TransformComponent>();
moonTransform.localPosition = glm::vec3(2.0f, 0.0f, 0.0f);
moonTransform.localScale = glm::vec3(0.3f);

// In your update loop
void Update(float dt)
{
    // Rotate sun
    auto& sunTransform = sun.get<TransformComponent>();
    glm::vec3 sunEuler = sunTransform.getEulerRotation();
    sunEuler.y += dt * 0.5f; // Slow rotation
    sunTransform.setRotationFromEuler(sunEuler);
    SceneGraph::MarkTransformDirty(sun);

    // Rotate Earth around sun (already handled by hierarchy!)
    // Just rotate Earth locally
    auto& earthTransform = earth.get<TransformComponent>();
    glm::vec3 earthEuler = earthTransform.getEulerRotation();
    earthEuler.y += dt * 1.0f;
    earthTransform.setRotationFromEuler(earthEuler);
    SceneGraph::MarkTransformDirty(earth);

    // Moon automatically orbits Earth due to hierarchy!
}
```

## Legacy Compatibility

The `m_trans` field in TransformComponent is maintained for backward compatibility and is automatically updated to match `worldMatrix`. If you have existing code using `m_trans`, it will continue to work.

## Performance Notes

1. **Dirty Flag Optimization**: Transforms are only recalculated when marked dirty
2. **Update Frequency**: System runs at 60 FPS by default (configurable)
3. **Hierarchy Depth**: Deeper hierarchies take longer to update (each level requires matrix multiplication)
4. **Batch Updates**: If you're modifying multiple entities, mark them all dirty at once

## System Configuration

The TransformHierarchySystem is automatically registered. To enable/disable it or change update rate, modify the system configuration in your engine config file.

```yaml
systems:
  TransformHierarchySystem:
    enabled: true
    update_rate: 60.0  # Updates per second
```

## Troubleshooting

### Transforms not updating
- Make sure you call `SceneGraph::MarkTransformDirty()` after modifying local transform
- Verify TransformHierarchySystem is enabled
- Check that entities have both TransformComponent and RelationshipComponent

### Unexpected transform values
- Remember that `localPosition/Rotation/Scale` are relative to parent
- Use `worldMatrix` to get absolute transform
- Verify parent-child relationships are set up correctly

### Performance issues
- Reduce hierarchy depth where possible
- Consider flattening deep hierarchies for static objects
- Mark only changed entities as dirty, not entire hierarchies
