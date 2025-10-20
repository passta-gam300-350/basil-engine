# Material Instance System

## Overview

The Material Instance System implements Unity-style per-entity material customization. It allows individual entities to override material properties without affecting the shared material asset used by other entities.

## Key Concepts

### Shared Material vs Material Instance

- **Shared Material**: The base material asset referenced by multiple entities
  - Changes affect ALL entities using this material
  - Memory efficient (single copy in memory)
  - Best for static, unchanging materials

- **Material Instance**: Per-entity copy that inherits from base material
  - Changes affect ONLY the entity with the instance
  - Stores only overridden properties (memory efficient)
  - Created automatically on first property modification
  - Falls back to base material for non-overridden properties

## Architecture

### Components

```
MaterialInstance
├── References: Base Material (shared)
├── Stores: Only overridden properties
└── Renders: Base properties + overrides

MaterialInstanceManager
├── Manages: Instance lifecycle per entity
├── Provides: Unity-style API
└── Cleanup: Automatic when entities destroyed
```

## Usage

### Basic Example: Per-Entity Material Customization

```cpp
#include <Resources/Material.h>
#include <Resources/MaterialInstance.h>
#include <Resources/MaterialInstanceManager.h>
#include <Render/Render.h>
#include <Engine.hpp>

// Assume we have entities e1, e2, e3 with MeshRendererComponents
// All referencing the same shared material "Metal"

auto& renderSystem = Engine::GetRenderSystem();
auto* instanceManager = renderSystem.GetMaterialInstanceManager();

// Get shared material (from ResourceManager or cache)
auto metalMaterial = resourceManager->GetMaterial("Metal");

// === Entity 1: Use shared material (no customization) ===
// Just keep using the shared material reference
// No instance needed

// === Entity 2: Override roughness only ===
auto instance2 = instanceManager->GetMaterialInstance(e2, metalMaterial);
instance2->SetFloat("u_RoughnessValue", 0.1f); // Smoother than base

// === Entity 3: Override albedo color ===
auto instance3 = instanceManager->GetMaterialInstance(e3, metalMaterial);
instance3->SetVec3("u_AlbedoColor", glm::vec3(1.0f, 0.0f, 0.0f)); // Red

// When rendering:
// - Entity 1 uses shared material (default metal look)
// - Entity 2 uses instance (same metal but smoother)
// - Entity 3 uses instance (red metal)
```

### Advanced Example: Runtime Property Animation

```cpp
void AnimateEntityMaterial(entt::entity entity, float time) {
    auto& renderSystem = Engine::GetRenderSystem();
    auto* instanceManager = renderSystem.GetMaterialInstanceManager();
    auto baseMaterial = GetEntityBaseMaterial(entity);

    // Get or create instance
    auto instance = instanceManager->GetMaterialInstance(entity, baseMaterial);

    // Animate emission
    float pulse = (std::sin(time * 2.0f) + 1.0f) * 0.5f; // 0-1
    instance->SetVec4("u_EmissiveColor", glm::vec4(1.0f, 0.5f, 0.0f, pulse));

    // Check if property is overridden
    if (instance->IsPropertyOverridden("u_RoughnessValue")) {
        spdlog::info("Entity {} has roughness override", static_cast<uint32_t>(entity));
    }
}
```

### Reverting to Shared Material

```cpp
// Option 1: Destroy the instance (entity will use shared material again)
instanceManager->DestroyInstance(entity);

// Option 2: Clear all overrides (keeps instance but resets to base)
auto instance = instanceManager->GetExistingInstance(entity);
if (instance) {
    instance->ClearOverrides();
}

// Option 3: Set a new shared material (destroys instance)
instanceManager->SetSharedMaterial(entity, newBaseMaterial);
```

### Checking Instance Status

```cpp
// Check if entity has an instance
if (instanceManager->HasInstance(entity)) {
    auto instance = instanceManager->GetExistingInstance(entity);

    // Check if instance has any overrides
    if (instance->HasOverrides()) {
        spdlog::info("Entity has {} overridden properties", /* count */);
    }
}
```

## API Reference

### MaterialInstance Class

```cpp
// Property Setters (Create Overrides)
void SetFloat(const std::string& name, float value);
void SetVec3(const std::string& name, const glm::vec3& value);
void SetVec4(const std::string& name, const glm::vec4& value);
void SetMat4(const std::string& name, const glm::mat4& value);

// Property Getters (With Fallback to Base)
float GetFloat(const std::string& name, float defaultValue = 0.0f) const;
glm::vec3 GetVec3(const std::string& name, const glm::vec3& defaultValue = glm::vec3(0.0f)) const;
glm::vec4 GetVec4(const std::string& name, const glm::vec4& defaultValue = glm::vec4(0.0f)) const;
glm::mat4 GetMat4(const std::string& name, const glm::mat4& defaultValue = glm::mat4(1.0f)) const;

// Instance Management
bool IsPropertyOverridden(const std::string& name) const;
bool HasOverrides() const;
void ClearOverrides();
std::shared_ptr<Material> GetBaseMaterial() const;
std::shared_ptr<Shader> GetShader() const;

// Rendering
void ApplyProperties(); // Apply all properties to shader
```

### MaterialInstanceManager Class

```cpp
// Instance Creation/Retrieval
std::shared_ptr<MaterialInstance> GetMaterialInstance(entt::entity entity, std::shared_ptr<Material> baseMaterial);
std::shared_ptr<MaterialInstance> GetExistingInstance(entt::entity entity) const;
bool HasInstance(entt::entity entity) const;

// Shared Material Management
std::shared_ptr<Material> GetSharedMaterial(entt::entity entity, std::shared_ptr<Material> baseMaterial) const;
void SetSharedMaterial(entt::entity entity, std::shared_ptr<Material> baseMaterial);

// Instance Destruction
void DestroyInstance(entt::entity entity);
void ClearAllInstances();

// Maintenance
void CleanupDestroyedEntities(entt::registry& registry);
size_t GetInstanceCount() const;
```

## Integration with MeshRendererComponent

### Current Approach (Manual)

```cpp
// In your render loop
auto view = registry.view<MeshRendererComponent, TransformComponent>();
for (auto entity : view) {
    auto& meshRenderer = view.get<MeshRendererComponent>(entity);

    // Get base material from GUID
    auto baseMaterial = resourceManager->GetMaterial(meshRenderer.m_MaterialGuid);

    // Check if entity wants to use an instance
    bool useInstance = /* your logic here */;

    if (useInstance) {
        auto instance = instanceManager->GetMaterialInstance(entity, baseMaterial);
        instance->ApplyProperties(); // Apply to shader
    } else {
        baseMaterial->ApplyAllProperties(); // Use shared material
    }

    // ... submit renderable ...
}
```

### Future: Automatic Integration (Recommended)

Consider adding these fields to `MeshRendererComponent`:

```cpp
struct MeshRendererComponent {
    // ... existing fields ...

    bool useInstance = false;  // Flag: use instance or shared material

    // Helper methods
    std::shared_ptr<Material> GetMaterial(MaterialInstanceManager* manager) const {
        auto baseMaterial = /* get from m_MaterialGuid */;
        if (useInstance) {
            return manager->GetMaterialInstance(entity, baseMaterial);
        }
        return baseMaterial;
    }
};
```

## Performance Considerations

### Memory Efficiency

- **Shared Material**: ~500 bytes (shader + property storage)
- **Material Instance**: ~200 bytes (only override storage + base reference)
- **Example**: 100 entities with same material
  - All shared: 500 bytes
  - All instances: 500 + (100 × 200) = 20,500 bytes
  - Mixed (50 shared, 50 instances): 500 + (50 × 200) = 10,500 bytes

### Performance Best Practices

1. **Use Shared Materials by Default**
   - Only create instances when customization needed
   - Reduces memory and GPU state changes

2. **Batch by Material**
   - Sort entities by base material before rendering
   - Minimizes shader switches

3. **Cleanup Regularly**
   ```cpp
   // In scene unload or major transitions
   instanceManager->CleanupDestroyedEntities(registry);
   ```

4. **Limit Override Count**
   - Fewer overrides = less memory per instance
   - Prefer dedicated materials for heavily customized entities

## Comparison with Unity

| Feature | Unity | This Engine | Notes |
|---------|-------|-------------|-------|
| Shared Material | `Renderer.sharedMaterial` | Base `Material` | ✅ Same concept |
| Material Instance | `Renderer.material` | `MaterialInstance` | ✅ Same concept |
| Copy-on-Write | Automatic | Manual via `GetMaterialInstance()` | ⚠️ Explicit in our engine |
| Property Fallback | Yes | Yes | ✅ Same behavior |
| Memory Pooling | Internal | Not yet implemented | 🔄 Future optimization |

## Future Enhancements

### Phase 4: Automatic Copy-on-Write

Add a wrapper that automatically creates instances when properties are modified:

```cpp
class MaterialHandle {
    entt::entity m_Entity;
    std::shared_ptr<Material> m_BaseMaterial;
    MaterialInstanceManager* m_Manager;

    // Automatically creates instance on first write
    void SetFloat(const std::string& name, float value) {
        auto instance = m_Manager->GetMaterialInstance(m_Entity, m_BaseMaterial);
        instance->SetFloat(name, value);
    }
};
```

### Phase 5: Serialization

Store instance overrides in scene files:

```json
{
  "entity": 12345,
  "components": {
    "MeshRendererComponent": {
      "materialGuid": "metal_base",
      "materialInstance": {
        "overrides": {
          "u_RoughnessValue": 0.1,
          "u_AlbedoColor": [1.0, 0.0, 0.0]
        }
      }
    }
  }
}
```

## Troubleshooting

### Instance Not Applying

```cpp
// Make sure to call ApplyProperties() before rendering
instance->ApplyProperties();
```

### Memory Leak Warning

```cpp
// Clean up instances when entities destroyed
instanceManager->CleanupDestroyedEntities(registry);
```

### Property Not Overriding

```cpp
// Check if property name matches shader uniform
auto shader = instance->GetShader();
shader->Use();
// Verify uniform exists: glGetUniformLocation(shader->ID, "u_PropertyName")
```

## Example Use Cases

1. **Character Customization**
   - Each character entity uses shared "Character" material
   - Instances override albedo color for team colors

2. **Damage States**
   - Objects share "Metal" material
   - Damaged objects instance with darker albedo

3. **Selection Highlighting**
   - Selected entities instance with emissive color override

4. **LOD Materials**
   - Far entities use shared material
   - Near entities instance with higher quality properties

---

**Last Updated:** 2025-10-20
**Status:** ✅ Fully Implemented (Phase 3 Complete)
