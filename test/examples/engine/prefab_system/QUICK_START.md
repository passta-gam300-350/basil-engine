# Prefab System - Quick Start Guide

## 🎯 What I Created For You

I've created a complete example suite to test your prefab system:

```
test/examples/engine/prefab_system/
├── minimal_test.cpp          # ⚡ Start here - tests core data structures
├── main.cpp                  # 🎮 Full example with ECS integration
├── sample_player.prefab      # 📄 Example of expected YAML format
├── CMakeLists.txt            # 🔧 Build configuration
├── README.md                 # 📚 Full documentation
└── QUICK_START.md           # 👈 You are here
```

## ⚡ Quick Test (2 minutes)

**Step 1: Build the minimal test**

```bash
cd test/examples/engine/prefab_system
mkdir build && cd build
cmake ..
cmake --build . --target prefab_minimal_test
```

**Step 2: Run it**

```bash
./prefab_minimal_test
```

**Expected output:**

```
╔════════════════════════════════════════╗
║  Prefab System - Minimal Test Suite   ║
╚════════════════════════════════════════╝

=== Testing UUID Round-Trip ===
Generated UUID: 550e8400-e29b-41d4-a716-446655440000
Parsed UUID:    550e8400-e29b-41d4-a716-446655440000
✓ UUID round-trip successful!

=== Testing UUID Parsing ===
✓ UUID parsing test passed!

=== Testing PrefabData Structure ===
✓ PrefabData structure test passed!

=== Testing PropertyOverride ===
✓ PropertyOverride test passed!

=== Testing PrefabEntity ===
✓ PrefabEntity test passed!

╔════════════════════════════════════════╗
║  ✓ All Tests Passed!                  ║
╚════════════════════════════════════════╝
```

If you see this, **your prefab system core is working!** ✓

## 🎮 Full Test (requires reflection setup)

**Step 1: Set up reflection registration**

Before running the full example, you need to register your components:

```cpp
// In your engine initialization code:
ReflectionRegistry::RegisterType<TransformComponent>()
    .RegisterField("m_Translation", &TransformComponent::m_Translation)
    .RegisterField("m_Scale", &TransformComponent::m_Scale)
    .RegisterField("m_Rotation", &TransformComponent::m_Rotation)
    .RegisterField("isDirty", &TransformComponent::isDirty);
```

**Step 2: Build the full example**

```bash
cmake --build . --target prefab_example
```

**Step 3: Run it**

```bash
./prefab_example
```

This will test:
- Creating prefabs from entities
- Saving/loading from disk
- Instantiating multiple instances
- Property overrides
- Querying instances

## 📋 What Each Test Does

### minimal_test.cpp (Start Here!)

Tests the **core data structures** without needing the full engine:

- ✓ `TestUUIDRoundTrip()` - Generate UUID → ToString() → FromString() → compare
- ✓ `TestUUIDParsing()` - Parse standard UUID format
- ✓ `TestPrefabDataStructure()` - Create PrefabData, add components
- ✓ `TestPropertyOverride()` - Create and verify overrides
- ✓ `TestPrefabEntity()` - Component storage and retrieval

**Dependencies:** Just UUID and GLM headers (no ECS, no YAML, no reflection)

### main.cpp (Full Integration)

Tests the **complete prefab workflow**:

1. Create entity hierarchy
2. Convert to prefab with `CreatePrefabFromEntity()`
3. Save to disk as YAML
4. Load from disk
5. Instantiate multiple instances
6. Modify instances (property overrides)
7. Query instances
8. Sync prefabs

**Dependencies:** Full engine (ECS, reflection, YAML, scene graph)

## 🔍 What Does Success Look Like?

### Minimal Test Success ✓

```
╔════════════════════════════════════════╗
║  ✓ All Tests Passed!                  ║
╚════════════════════════════════════════╝
```

**This proves:**
- UUID parsing/generation works
- PrefabData structures are correct
- Property overrides work
- Component storage works

### Full Example Success ✓

```
========== Example Complete ==========

The prefab system is working! ✓

Generated files:
  - player.prefab (YAML prefab data)
```

**This proves:**
- Reflection integration works
- YAML serialization works
- ECS integration works
- Instantiation works
- All APIs work end-to-end

## 🚨 Troubleshooting

### "UUID methods not found"

**Problem:** UUID::FromString() not implemented
**Solution:** You're reading this AFTER I implemented it, so it should work. If not, check `lib/uuid/include/uuid/uuid.hpp` lines 72 and 241-287.

### "Reflection type not found"

**Problem:** Components not registered with reflection
**Solution:** Add reflection registration before running (see step 1 of Full Test)

### "Cannot open YAML file"

**Problem:** Working directory is wrong
**Solution:** Run from the build directory, or provide absolute paths

## 📊 Test Coverage

| Feature | Minimal Test | Full Example |
|---------|--------------|--------------|
| UUID generation | ✓ | ✓ |
| UUID parsing | ✓ | ✓ |
| PrefabData structure | ✓ | ✓ |
| Property overrides | ✓ | ✓ |
| Component storage | ✓ | ✓ |
| Reflection serialization | ✗ | ✓ |
| YAML save/load | ✗ | ✓ |
| ECS integration | ✗ | ✓ |
| Instantiation | ✗ | ✓ |
| Instance queries | ✗ | ✓ |
| Prefab syncing | ✗ | ✓ |

## 🎯 Recommended Testing Order

1. **Run minimal_test** ← Start here (fastest, no setup)
2. **Inspect sample_player.prefab** ← See expected YAML format
3. **Set up reflection** ← Required for full example
4. **Run main example** ← Full integration test
5. **Inspect generated player.prefab** ← Verify your serialization
6. **Try modifying the example** ← Add your own components

## 📖 Next Steps

After both tests pass:

1. **Integrate into your editor** - Wire up prefab creation/instantiation UI
2. **Add more components** - Test with MeshRenderer, Colliders, etc.
3. **Test hierarchies** - Create prefabs with parent-child relationships
4. **Test syncing** - Modify a prefab and sync all instances
5. **Write unit tests** - Use these examples as templates

## 💡 Tips

- **Start with minimal_test** - It's the fastest way to verify core functionality
- **Check the YAML files** - They're human-readable and show exactly what's being saved
- **Use the debugger** - Set breakpoints in PrefabSystem.cpp to see the flow
- **Read the comments** - Both examples are heavily documented

## ❓ Common Questions

**Q: Why two separate tests?**
A: `minimal_test` verifies data structures quickly without complex setup. `main.cpp` tests the full system integration.

**Q: Do I need to run both?**
A: Run `minimal_test` first. If it fails, your core implementation has issues. If it passes but `main.cpp` fails, it's likely a reflection/YAML/ECS integration issue.

**Q: Can I modify these examples?**
A: Yes! They're templates. Add your components, test your use cases, etc.

**Q: How is this different from Unity?**
A: See the comparison table in README.md. TLDR: Very similar workflow, just more manual (no auto-sync).

---

**Ready to test?** Run `prefab_minimal_test` now! ⚡
