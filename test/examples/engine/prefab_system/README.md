# Prefab System Examples

This directory contains **self-contained, minimal tests** for the prefab system that work **without requiring full engine compilation**.

## Philosophy: Aligned with minimal_test Pattern

Both tests follow the same philosophy as `minimal_test.cpp`:
- **Self-contained**: No full engine dependencies
- **Focused**: Tests one layer at a time
- **Works independently**: Can compile and run without building the entire engine
- **Clear output**: Shows exactly what's being tested and what passed

## What's Here

```
test/examples/engine/prefab_system/
├── minimal_test.cpp          # Tests UUID core functionality (Phase 1)
├── main.cpp                  # Tests PrefabData structures and YAML (Phase 2)
├── sample_player.prefab      # Example of expected YAML format
├── CMakeLists.txt            # Build configuration
├── README.md                 # This file
└── QUICK_START.md           # Quick start guide
```

## Test 1: minimal_test.cpp (UUID Foundation)

### What It Tests

✓ UUID generation
✓ UUID ToString()
✓ UUID FromString() parsing
✓ UUID round-trip (generate → string → parse → compare)
✓ UUID equality comparison
✓ Uppercase/lowercase parsing
✓ Parsing with/without dashes

### Dependencies

**ZERO dependencies** - Only includes `uuid/uuid.hpp`

### How to Build

```bash
cd test/examples/engine/prefab_system
mkdir build && cd build
cmake ..
cmake --build . --target prefab_minimal_test
```

### How to Run

```bash
./prefab_minimal_test
```

### Expected Output

```
==========================================
   UUID System - Minimal Test Suite
   (Phase 2: UUID Parsing)
==========================================

=== Testing UUID Generation ===
UUID 1: 550e8400-e29b-41d4-a716-446655440000
UUID 2: 7c9e6679-7425-40de-944b-e07fc1f90ae7
[PASS] UUIDs are unique!

=== Testing UUID ToString() ===
Generated UUID: a3bb189e-8bf9-3888-9912-ace4e6543002
[PASS] UUID format is correct (8-4-4-4-12)!

=== Testing UUID FromString() ===
Parsing: 550e8400-e29b-41d4-a716-446655440000
Result:  550e8400-e29b-41d4-a716-446655440000
[PASS] UUID parsing successful!

... (more tests)

==========================================
  [PASS] All Tests Passed!
==========================================

What this proves:
[+] UUID generation works
[+] UUID ToString() produces correct format
[+] UUID FromString() parses correctly (Phase 2)
[+] Round-trip (generate -> string -> parse) preserves data
[+] UUID equality comparison works
[+] Both uppercase and lowercase parsing work
[+] Parsing works with or without dashes

This is the foundation for prefab UUID persistence.
Prefabs can now be saved with UUIDs and loaded back!
```

## Test 2: main.cpp (PrefabData & YAML Serialization)

### What It Tests

✓ Creating PrefabData structures
✓ Adding SerializedComponent to PrefabEntity
✓ Property storage and retrieval
✓ Writing prefab data to YAML files
✓ Reading prefab data from YAML files
✓ Round-trip serialization (create → save → load → compare)
✓ Entity hierarchy construction (parent-child relationships)

### Dependencies

**Minimal dependencies** - Only requires:
- `uuid/uuid.hpp` (UUID support)
- `Prefab/PrefabData.hpp` (data structures - header only)
- `yaml-cpp` (YAML serialization)
- `glm` (math types)

**Does NOT require:**
- ❌ Compiled engine library
- ❌ ECS world
- ❌ Reflection system
- ❌ Component implementations
- ❌ PrefabSystem (the full system)

### How to Build

```bash
cd test/examples/engine/prefab_system
mkdir build && cd build
cmake ..
cmake --build . --target prefab_example
```

### How to Run

```bash
./prefab_example
```

### Expected Output

```
==========================================
   PrefabData System - Minimal Test
   (Serialization & Data Structures)
==========================================

This test validates PrefabData structures
and YAML serialization without requiring
the full engine compilation.

========== Test 1: PrefabData Creation ==========
Created prefab with:
  Name: Player
  UUID: a3bb189e-8bf9-3888-9912-ace4e6543002
  Version: 1
  Created: 2025-10-29T12:00:00Z
  Components: 2
    - TransformComponent (hash: 123456789)
      Properties: 4
    - NameComponent (hash: 987654321)
      Properties: 1
[PASS] PrefabData creation works!

========== Test 2: Component Property Access ==========
TransformComponent properties:
  isDirty: true
  m_Rotation: (0, 0, 0)
  m_Scale: (1, 1, 1)
  m_Translation: (0, 0, 0)
[PASS] Component property access works!

========== Test 3: YAML Serialization ==========
Saving prefab to: test_prefab.prefab
[PASS] Prefab saved successfully!

========== Test 4: YAML Deserialization ==========
Loading prefab from: test_prefab.prefab
Loaded prefab:
  Name: Player
  UUID: a3bb189e-8bf9-3888-9912-ace4e6543002
  Version: 1
  Created: 2025-10-29T12:00:00Z
  Components: 2
    - TransformComponent (hash: 123456789)
      Properties: 4
    - NameComponent (hash: 987654321)
      Properties: 1
[PASS] Prefab loaded successfully!

========== Test 5: Round-Trip Serialization ==========
Original UUID: a3bb189e-8bf9-3888-9912-ace4e6543002
Loaded UUID:   a3bb189e-8bf9-3888-9912-ace4e6543002
[PASS] Round-trip preserves data!

========== Test 6: Load Sample Prefab ==========
Loading sample prefab: sample_player.prefab
Sample prefab loaded:
  Name: Player
  UUID: 550e8400-e29b-41d4-a716-446655440000
  Version: 1
  Created: 2025-10-29T12:00:00Z
  Components: 2
    - TransformComponent (hash: 123456789)
      Properties: 4
    - NameComponent (hash: 987654321)
      Properties: 1
[PASS] Sample prefab loaded!

========== Test 7: Entity Hierarchy ==========
Created hierarchy:
  Root components: 1
  Children: 1
  Child[0] components: 2
[PASS] Hierarchy serialization works!

==========================================
  [PASS] All Tests Passed!
==========================================

What this proves:
[+] PrefabData structures work correctly
[+] Component serialization works
[+] Property storage and retrieval work
[+] YAML serialization/deserialization work
[+] Round-trip preserves all data
[+] Entity hierarchies serialize correctly

Generated files:
  - test_prefab.prefab
  - test_hierarchy.prefab

This is the foundation for the prefab system.
Next steps:
1. Implement PrefabSystem::LoadPrefabFromFile() using this YAML code
2. Implement PrefabSystem::SavePrefabToFile() using this YAML code
3. Integrate with full engine and ECS for instantiation
```

## Generated Files

After running `prefab_example`, you'll see:

### test_prefab.prefab

```yaml
metadata:
  uuid: a3bb189e-8bf9-3888-9912-ace4e6543002
  name: Player
  version: 1
  created: 2025-10-29T12:00:00Z
root:
  components:
    - typeHash: 123456789
      typeName: TransformComponent
      properties:
        isDirty: true
        m_Rotation: [0, 0, 0]
        m_Scale: [1, 1, 1]
        m_Translation: [0, 0, 0]
    - typeHash: 987654321
      typeName: NameComponent
      properties:
        name: Player
  children: []
```

### test_hierarchy.prefab

```yaml
metadata:
  uuid: <generated-uuid>
  name: PlayerWithWeapon
  version: 1
  created: 2025-10-29T12:00:00Z
root:
  components:
    - typeHash: 123456789
      typeName: TransformComponent
      properties:
        m_Translation: [0, 0, 0]
        m_Scale: [1, 1, 1]
  children:
    - components:
        - typeHash: 123456789
          typeName: TransformComponent
          properties:
            m_Translation: [1, 0, 0]
            m_Scale: [0.5, 0.5, 0.5]
        - typeHash: 987654321
          typeName: NameComponent
          properties:
            name: Sword
      children: []
```

## What This Proves

### minimal_test Success ✓

**Proves:** The UUID system is working correctly and can:
- Generate unique UUIDs
- Convert UUIDs to/from strings
- Parse various UUID formats (with/without dashes, upper/lowercase)
- Maintain UUID identity through round-trips

**Foundation for:** Prefab identification and persistence

### prefab_example Success ✓

**Proves:** The PrefabData structures and YAML serialization work correctly:
- PrefabData can be created programmatically
- Components can be serialized with their properties
- YAML serialization preserves all data
- Hierarchies (parent-child relationships) serialize correctly
- Round-trip serialization maintains data integrity

**Foundation for:** The full PrefabSystem implementation (saving/loading prefabs)

## Integration Path

These tests are **foundational**. Here's how they fit into the full system:

```
Phase 1: UUID Foundation
└─ minimal_test.cpp ✓ (You are here)

Phase 2: PrefabData & Serialization
└─ main.cpp ✓ (You are here)

Phase 3: PrefabSystem Implementation
├─ Implement PrefabSystem::LoadPrefabFromFile()
│  └─ Use YAML code from main.cpp
├─ Implement PrefabSystem::SavePrefabToFile()
│  └─ Use YAML code from main.cpp
└─ Implement PrefabSystem::CreatePrefabFromEntity()
   └─ Use reflection to extract component data

Phase 4: Full Engine Integration
├─ InstantiatePrefab() - Create entities from prefab data
├─ SyncPrefab() - Update instances when prefab changes
├─ Property overrides - Track instance modifications
└─ Editor integration - UI for creating/editing prefabs
```

## Comparison to Original (Broken) Version

### Old main.cpp (BROKEN)

❌ Required full engine compilation
❌ Required ECS world implementation
❌ Required reflection system setup
❌ Required PrefabSystem to be fully implemented
❌ Had TODOs and commented-out code
❌ Couldn't compile without entire engine
❌ Not self-contained

### New main.cpp (WORKING)

✅ Self-contained like minimal_test
✅ Only requires headers and yaml-cpp
✅ Works without engine compilation
✅ Tests the data layer (PrefabData structures)
✅ No TODOs, fully functional
✅ Can run immediately
✅ Provides foundation for PrefabSystem implementation

## Troubleshooting

### Problem: "YAML::LoadFile failed"

**Solution:** Make sure you run from the build directory, or that `sample_player.prefab` is in your working directory.

### Problem: "UUID methods not found"

**Solution:** Ensure you have the Phase 2 UUID implementation with `FromString()`. Check `lib/uuid/include/uuid/uuid.inl` lines 241-287.

### Problem: "Cannot find PrefabData.hpp"

**Solution:** Check that `ENGINE_INCLUDE_DIR` in CMakeLists.txt points to the correct engine include directory.

### Problem: Compilation errors about variant/glm

**Solution:** Ensure you're using C++20 (`target_compile_features(... cxx_std_20)`) and that glm is properly linked.

## Next Steps

1. **Run both tests** to verify the foundation works
2. **Inspect the generated YAML files** to understand the serialization format
3. **Implement PrefabSystem::LoadPrefabFromFile()** using the YAML code from main.cpp
4. **Implement PrefabSystem::SavePrefabFromFile()** using the YAML code from main.cpp
5. **Copy the YAML converter templates** to your PrefabSystem implementation
6. **Test with the full engine** once PrefabSystem is implemented
7. **Add reflection integration** for automatic component serialization

## Key Takeaway

These tests prove that the **core data layer works**. They give you:

1. **Working YAML serialization code** - Copy the `YAML::convert<>` templates to your PrefabSystem
2. **Confidence in PrefabData** - The structures are correct and work as designed
3. **Foundation for PrefabSystem** - You now know the load/save logic works
4. **Fast iteration** - No need to compile the entire engine to test prefabs
5. **Clear success criteria** - If these tests pass, your foundation is solid

**The hard part (data structures and serialization) is done. Now you can implement the PrefabSystem with confidence!**
