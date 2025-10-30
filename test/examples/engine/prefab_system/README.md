# Prefab System Example

This example demonstrates the basic functionality of the GAM300 prefab system.

## What This Example Tests

1. ✓ **Creating entities** with TransformComponent and NameComponent
2. ✓ **Creating prefabs** from entity hierarchies via `CreatePrefabFromEntity()`
3. ✓ **Saving prefabs** to disk as YAML files
4. ✓ **Loading prefabs** from disk with UUID parsing
5. ✓ **Instantiating** multiple prefab instances at different positions
6. ✓ **Property overrides** (modifying instance-specific values)
7. ✓ **Querying instances** via `GetAllInstances()`, `IsInstanceOf()`, `IsPrefabInstance()`

## Prerequisites

Before running this example, you need to:

### 1. Register Components with Reflection System

The prefab system uses reflection to serialize/deserialize components. You need to register your components first. Add this to your reflection registry initialization:

```cpp
// In your reflection registry setup code:
ReflectionRegistry::RegisterType<TransformComponent>()
    .RegisterField("m_Translation", &TransformComponent::m_Translation)
    .RegisterField("m_Scale", &TransformComponent::m_Scale)
    .RegisterField("m_Rotation", &TransformComponent::m_Rotation)
    .RegisterField("isDirty", &TransformComponent::isDirty);

ReflectionRegistry::RegisterType<NameComponent>()
    .RegisterField("name", &NameComponent::name);
```

### 2. Ensure Dependencies Are Available

- EnTT (for ECS and reflection)
- GLM (for math types)
- yaml-cpp (for YAML serialization)
- UUID library (for prefab identification)

### 3. Implement SceneGraph::SetParent (if using hierarchies)

The prefab system calls `SceneGraph::SetParent()` when creating child entities. Make sure this is implemented in your scene graph system.

## Building

### Option 1: Using CMake

```bash
cd test/examples/engine/prefab_system
mkdir build
cd build
cmake ..
cmake --build .
```

### Option 2: Add to Main Project

Add this directory to your main CMakeLists.txt:

```cmake
add_subdirectory(test/examples/engine/prefab_system)
```

## Running

```bash
./prefab_example
```

### Expected Output

```
========== Prefab System Example ==========

========== Step 1: Create Entity Hierarchy ==========
Creating player entity with transform and name...
Player Template:
  Name: Player
  Position: (0, 0, 0)
  Scale: (1, 1, 1)

========== Step 2: Create Prefab from Entity ==========
Creating prefab at: player.prefab
Prefab created successfully!
Prefab UUID: 550e8400-e29b-41d4-a716-446655440000
Removed template entity

========== Step 3: Load Prefab from Disk ==========
Prefab loaded successfully!
  Name: Player
  UUID: 550e8400-e29b-41d4-a716-446655440000
  Version: 1
  Components: 2

========== Step 4: Instantiate Prefab ==========
Instantiating 3 player instances...
All instances created successfully!

Player 1:
  Name: Player
  Position: (0, 0, 0)
  Scale: (1, 1, 1)
  [Prefab Instance] GUID: 550e8400-e29b-41d4-a716-446655440000
  Override count: 0

Player 2:
  Name: Player
  Position: (5, 0, 0)
  Scale: (1, 1, 1)
  [Prefab Instance] GUID: 550e8400-e29b-41d4-a716-446655440000
  Override count: 0

Player 3:
  Name: Player
  Position: (10, 0, 0)
  Scale: (1, 1, 1)
  [Prefab Instance] GUID: 550e8400-e29b-41d4-a716-446655440000
  Override count: 0

========== Step 5: Create Property Override ==========
Modifying Player 2's scale to (2, 2, 2)...
Modified Player 2:
  Position: (5, 0, 0)
  Scale: (2, 2, 2)
  Override count: 1

========== Step 6: Query Prefab Instances ==========
Found 3 instances of the Player prefab
  Instance 1: ✓ Valid
  Instance 2: ✓ Valid
  Instance 3: ✓ Valid

========== Step 7: Check Instance Status ==========
Player 1 is prefab instance: Yes
Regular entity is prefab instance: No

========== Step 8: Sync Instances ==========
Note: SyncPrefab would reload from disk and update all instances
while preserving property overrides.

========== Cleanup ==========
Prefab system shut down successfully

========== Example Complete ==========

The prefab system is working! ✓

Generated files:
  - player.prefab (YAML prefab data)

You can inspect the .prefab file to see the serialized format.
```

## Generated Files

After running, you'll see `player.prefab` in the working directory. It should look like:

```yaml
metadata:
  uuid: "550e8400-e29b-41d4-a716-446655440000"
  name: "Player"
  version: 1
  created: "2025-10-29T12:00:00Z"

root:
  components:
    - typeHash: 123456789
      typeName: "TransformComponent"
      properties:
        m_Translation: [0.0, 0.0, 0.0]
        m_Scale: [1.0, 1.0, 1.0]
        m_Rotation: [0.0, 0.0, 0.0]
        isDirty: true
    - typeHash: 987654321
      typeName: "NameComponent"
      properties:
        name: "Player"
  children: []
```

## Troubleshooting

### Problem: "Reflection type not found"

**Solution**: Make sure you've registered all components with the reflection system before calling `PrefabSystem::Init()`.

### Problem: "Failed to load prefab from disk"

**Solution**:
1. Check that the .prefab file exists
2. Verify YAML syntax is correct
3. Ensure yaml-cpp library is linked

### Problem: "Failed to instantiate prefab"

**Solution**:
1. Check that the prefab was loaded successfully
2. Verify all component types in the prefab are registered with reflection
3. Check that the ECS world is properly initialized

### Problem: Compilation errors about missing methods

**Solution**: Make sure you've implemented:
- `UUID<128>::FromString()` - Added in Phase 2
- `UUID<128>::ToString()` - Should already exist
- Reflection bridge functions in `PrefabSystem.cpp`

## Next Steps

After verifying this basic example works:

1. **Add child entities**: Test hierarchical prefabs with parent-child relationships
2. **Test property overrides**: Modify instances and sync them
3. **Test Apply to Prefab**: Update the prefab from an instance
4. **Add more components**: Try with MeshRenderer, physics components, etc.
5. **Write unit tests**: Use this example as a template for automated tests

## Comparison to Unity

This example is roughly equivalent to Unity's workflow:

| Unity | GAM300 |
|-------|--------|
| Create GameObject in scene | `world.add_entity()` + add components |
| Drag to Project → Create Prefab | `CreatePrefabFromEntity()` |
| Drag prefab to scene | `InstantiatePrefab()` |
| Modify instance in Inspector | Modify components + `MarkPropertyOverridden()` |
| Apply → "Apply to Prefab" | `ApplyInstanceToPrefab()` |
| Prefab changes auto-sync | `SyncPrefab()` (manual for now) |

## Known Limitations

- **No nested prefabs**: Prefabs cannot contain other prefabs (matches your requirement)
- **No prefab variants**: All instances share the same base prefab
- **Manual syncing**: Unlike Unity's automatic sync, you must call `SyncPrefab()` manually
- **No undo/redo**: Property overrides can be reverted but there's no undo stack
