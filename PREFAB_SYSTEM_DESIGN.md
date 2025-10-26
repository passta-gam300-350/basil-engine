# Prefab System Design

## Overview

This document outlines the architecture for the prefab system in GAM300, including prefab creation, instantiation, syncing, and override management.

## Architecture Philosophy

The prefab system follows the separation between **runtime engine** and **authoring editor**:

- **Engine Side**: Runtime components, systems, and core logic that work in both editor and game
- **Editor Side**: Authoring tools, asset management, and UI for prefab manipulation

## Core Concepts

### Prefab
A reusable template containing:
- Entity hierarchy (parent-child relationships)
- Component data for each entity
- Metadata (UUID, name, creation date)

### Prefab Instance
An entity in the scene that references a prefab:
- Links to source prefab via UUID
- Can override specific properties
- Automatically syncs when prefab changes

### Property Override
Individual component properties that differ from the prefab template:
- Tracked per-instance
- Preserved during prefab sync
- Can be reverted to prefab values

## Engine Side Implementation

### Location
`engine/include/Component/PrefabComponent.hpp`
`engine/include/System/PrefabSystem.hpp`

### Components

#### PrefabInstanceComponent
```cpp
struct PrefabInstanceComponent {
    UUID<128> prefabId;                        // Source prefab reference
    std::unordered_set<uint32_t> overriddenProperties; // Hashed property paths
    bool autoSync = true;                      // Auto-apply prefab changes
};
```

#### PrefabRootComponent
```cpp
struct PrefabRootComponent {
    UUID<128> prefabId;                        // For the root entity of a prefab instance
    std::vector<ecs::entity> prefabEntities;   // All entities in this prefab instance
};
```

### Systems

#### PrefabSystem
Responsibilities:
- **Instantiate prefabs** - Create entity hierarchy from prefab data
- **Sync instances** - Apply prefab changes to instances
- **Track overrides** - Detect and preserve modified properties
- **Manage lifecycle** - Handle prefab loading/unloading

Key Methods:
```cpp
class PrefabSystem {
public:
    void Init();
    void Update(ecs::world& world, float deltaTime);
    void Exit();

    // Core operations
    ecs::entity InstantiatePrefab(const UUID<128>& prefabId, const glm::vec3& position);
    void SyncPrefab(const UUID<128>& prefabId);
    void SyncInstance(ecs::entity instance);

    // Override management
    void MarkPropertyOverridden(ecs::entity instance, uint32_t propertyHash);
    void RevertOverride(ecs::entity instance, uint32_t propertyHash);
    void RevertAllOverrides(ecs::entity instance);

    // Queries
    std::vector<ecs::entity> GetAllInstances(const UUID<128>& prefabId);
    bool IsInstanceOf(ecs::entity entity, const UUID<128>& prefabId);
};
```

### Prefab Data Format

Stored as JSON or binary format:
```json
{
    "metadata": {
        "uuid": "550e8400-e29b-41d4-a716-446655440000",
        "name": "EnemyTank",
        "version": 1,
        "created": "2025-10-10T12:00:00Z"
    },
    "root": {
        "components": {
            "TransformComponent": {
                "localPosition": [0, 0, 0],
                "localRotation": [0, 0, 0, 1],
                "localScale": [1, 1, 1]
            },
            "RelationshipComponent": {}
        },
        "children": [
            {
                "components": {
                    "TransformComponent": {
                        "localPosition": [0, 1, 0]
                    }
                },
                "children": []
            }
        ]
    }
}
```

## Editor Side Implementation

### Location
`editor/include/Manager/PrefabManager.hpp`
`editor/include/Panels/PrefabInspectorPanel.hpp` (new)

### PrefabManager

Extends editor's asset management system:

```cpp
class PrefabManager {
public:
    // Asset operations
    bool CreatePrefabFromSelection(const std::string& path);
    bool SavePrefab(const UUID<128>& prefabId, const std::string& path);
    UUID<128> LoadPrefab(const std::string& path);

    // Change detection
    void MarkPrefabDirty(const UUID<128>& prefabId);
    void OnPrefabModified(const UUID<128>& prefabId);

    // Integration with AssetManager
    void RegisterPrefabAssetType();
};
```

### UI Components

#### Hierarchy Panel Additions
- Visual indicator for prefab instances (e.g., blue text color)
- Right-click menu: "Select Prefab", "Unpack Prefab", "Revert Prefab"
- Prefab root has special icon

#### Inspector Panel Additions
- Property override indicators (bold text or colored background)
- Per-property revert buttons
- "Apply to Prefab" button (saves instance changes to prefab)
- "Revert Prefab" button (discards all overrides)

#### New: Prefab Inspector Panel
When editing a prefab asset directly:
- Isolated editing environment
- Preview of prefab hierarchy
- "Save Prefab" button
- Instance count display

## Workflows

### Creating a Prefab

1. **User Action**: Select entity/hierarchy in scene → Right-click → "Create Prefab"
2. **Editor**: `PrefabManager::CreatePrefabFromSelection(path)`
   - Serialize selected hierarchy
   - Generate UUID
   - Save to `.prefab` file
3. **Editor**: Replace scene entities with prefab instance
   - Call `PrefabSystem::InstantiatePrefab(uuid)`
   - Delete original entities

### Editing a Prefab

**Option 1: Edit via Instance**
1. Modify instance properties in scene
2. Click "Apply to Prefab" in inspector
3. Editor saves changes to `.prefab` file
4. Editor calls `PrefabSystem::SyncPrefab(uuid)`
5. All other instances update automatically

**Option 2: Edit Prefab Asset Directly**
1. Double-click `.prefab` in asset browser
2. Opens Prefab Editor mode
3. Edit in isolated environment
4. Click "Save"
5. Editor calls `PrefabSystem::SyncPrefab(uuid)`

### Instantiating a Prefab

1. **User Action**: Drag `.prefab` from asset browser to scene
2. **Editor**: Calls `PrefabSystem::InstantiatePrefab(uuid, position)`
3. **Engine**:
   - Load prefab data from PrefabManager
   - Create entity hierarchy
   - Add `PrefabInstanceComponent` to root
   - Apply initial transforms

### Syncing Prefab Changes

1. **Trigger**: Prefab asset modified
2. **Editor**: `PrefabManager::OnPrefabModified(uuid)`
3. **Engine**: `PrefabSystem::SyncPrefab(uuid)`
   - Find all instances with matching `prefabId`
   - For each instance:
     - Load new prefab data
     - Apply changes to non-overridden properties
     - Preserve overridden properties
     - Maintain local transforms

### Override Management

**Marking Override**:
1. User modifies property in inspector
2. Editor calls `PrefabSystem::MarkPropertyOverridden(entity, hash)`
3. Property hash added to `PrefabInstanceComponent::overriddenProperties`

**Property Hash Generation**:
```cpp
uint32_t hash = Hash("TransformComponent.localPosition.x");
```

**Reverting Override**:
1. User clicks revert button next to property
2. Editor calls `PrefabSystem::RevertOverride(entity, hash)`
3. Property restored from prefab data
4. Hash removed from override set

## Integration with Scene Graph

The prefab system builds on top of the existing scene graph:

- Prefab hierarchies use `RelationshipComponent` and `SceneGraph` API
- Transform propagation handled by `HierarchySystem`
- Prefab sync respects parent-child relationships

Example: When syncing a prefab instance:
```cpp
void PrefabSystem::SyncInstance(ecs::entity root) {
    // Get prefab data
    auto& prefabComp = root.get<PrefabInstanceComponent>();
    PrefabData data = LoadPrefabData(prefabComp.prefabId);

    // Sync root
    ApplyPrefabData(root, data.root, prefabComp.overriddenProperties);

    // Recursively sync children using SceneGraph
    auto children = SceneGraph::GetChildren(root);
    for (size_t i = 0; i < children.size(); ++i) {
        SyncEntity(children[i], data.root.children[i], prefabComp.overriddenProperties);
    }
}
```

## Implementation Phases

### Phase 1: Engine Core (Week 1)
- [ ] Create `PrefabInstanceComponent` and `PrefabRootComponent`
- [ ] Implement basic `PrefabSystem` structure
- [ ] Add prefab data serialization/deserialization
- [ ] Implement `InstantiatePrefab()` functionality
- [ ] Write unit tests for prefab instantiation

### Phase 2: Syncing & Overrides (Week 2)
- [ ] Implement `SyncPrefab()` and `SyncInstance()`
- [ ] Add property override tracking
- [ ] Implement override detection logic
- [ ] Add `RevertOverride()` functionality
- [ ] Write tests for sync and overrides

### Phase 3: Editor Integration (Week 3)
- [ ] Create `PrefabManager` in editor
- [ ] Integrate with `AssetManager` for `.prefab` files
- [ ] Add prefab indicators in hierarchy panel
- [ ] Add override indicators in inspector
- [ ] Implement "Create Prefab" workflow

### Phase 4: Advanced Features (Week 4)
- [ ] Prefab editor mode (isolated editing)
- [ ] "Apply to Prefab" from instance
- [ ] Nested prefab support (prefabs within prefabs)
- [ ] Prefab variants (inherit from base prefab)
- [ ] Auto-sync on asset reload

## Testing Strategy

### Unit Tests
- Prefab instantiation creates correct hierarchy
- Sync updates non-overridden properties only
- Override tracking persists across syncs
- Revert operations restore prefab values

### Integration Tests
- Create prefab from scene hierarchy
- Modify prefab and verify all instances update
- Override properties and verify they persist
- Scene save/load with prefab instances

### Example Test Scene
Create `test/examples/engine/prefab/main.cpp`:
- Create a prefab with 3-level hierarchy
- Instantiate 5 instances
- Modify prefab and verify sync
- Override properties on instances
- Verify overrides persist after sync

## Future Enhancements

- **Prefab Variants**: Inherit from base prefab with modifications
- **Nested Prefabs**: Use prefabs as children within other prefabs
- **Prefab Collections**: Group related prefabs
- **Visual Diff Tool**: Show differences between instance and prefab
- **Batch Operations**: Sync/revert multiple instances at once
- **Prefab Dependencies**: Track which prefabs use other prefabs
- **Version Control**: Handle prefab merges in git

## References

- Scene Graph: `engine/include/Scene/SceneGraph.hpp`
- Hierarchy System: `engine/include/System/HierarchySystem.hpp`
- ECS Integration: `test/examples/engine/scene_graph/main.cpp`
- Asset Manager: `editor/include/Manager/AssetManager.hpp`
