# Material Override System - Testing Guide

## Overview

The Material Override System allows you to customize material properties per-entity without breaking GPU instancing. This follows Unity's MaterialPropertyBlock pattern.

## How It Works

1. **MeshRendererComponent**: References a base material (shared by multiple entities)
2. **MaterialOverridesComponent**: Stores per-entity property overrides
3. **MaterialOverridesSystem**: Creates MaterialPropertyBlocks at runtime with the overrides applied
4. **SceneRenderer**: Applies property blocks after base material during rendering (preserves GPU instancing!)

## Shader Property Names

Your shader uniforms must match these exact names for overrides to work:

### PBR Material Properties
- `u_AlbedoColor` (vec3) - Base color
- `u_MetallicValue` (float) - Metallic property (0.0 = dielectric, 1.0 = metal)
- `u_RoughnessValue` (float) - Surface roughness (0.0 = smooth, 1.0 = rough)
- `u_EmissionColor` (vec3) - Emissive color
- `u_AoValue` (float) - Ambient occlusion

### Texture Maps
- `u_AlbedoMap` - Albedo/diffuse texture
- `u_NormalMap` - Normal map
- `u_MetallicMap` - Metallic map
- `u_RoughnessMap` - Roughness map
- `u_AoMap` - Ambient occlusion map

## Example Usage in Code

```cpp
// Create an entity with a mesh
ecs::world world = Engine::GetWorld();
auto entity = world.add_entity();

// Add transform and mesh renderer
world.add_component_to_entity<TransformComponent>(entity,
    TransformComponent{glm::vec3{1,1,1}, glm::vec3{}, glm::vec3(0,0,0)});

MeshRendererComponent meshRenderer;
meshRenderer.isPrimitive = true;
meshRenderer.m_PrimitiveType = MeshRendererComponent::PrimitiveType::CUBE;
meshRenderer.hasAttachedMaterial = false;
meshRenderer.m_MaterialGuid = Resource::Guid{}; // Default material
world.add_component_to_entity<MeshRendererComponent>(entity, meshRenderer);

// Add material overrides to customize this specific entity
MaterialOverridesComponent overrides;

// Override float properties
overrides.floatOverrides["u_MetallicValue"] = 0.9f;  // Very metallic
overrides.floatOverrides["u_RoughnessValue"] = 0.2f; // Fairly smooth

// Override vec3 properties (colors)
overrides.vec3Overrides["u_AlbedoColor"] = glm::vec3(1.0f, 0.0f, 0.0f); // Red
overrides.vec3Overrides["u_EmissionColor"] = glm::vec3(0.5f, 0.0f, 0.0f); // Red glow

// Override vec4 properties (if needed)
overrides.vec4Overrides["u_CustomColor"] = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);

// Override textures (provide texture GUID)
// overrides.textureOverrides["u_AlbedoMap"] = someTextureGuid;

world.add_component_to_entity<MaterialOverridesComponent>(entity, overrides);
```

## Testing Material Overrides

### Test 1: Different Colors on Same Mesh Type
Create multiple cubes with different albedo colors:

```cpp
// Red cube
MaterialOverridesComponent red;
red.vec3Overrides["u_AlbedoColor"] = glm::vec3(1.0f, 0.0f, 0.0f);
red.floatOverrides["u_MetallicValue"] = 0.0f;
red.floatOverrides["u_RoughnessValue"] = 0.8f;

// Blue metal cube
MaterialOverridesComponent blue;
blue.vec3Overrides["u_AlbedoColor"] = glm::vec3(0.0f, 0.3f, 1.0f);
blue.floatOverrides["u_MetallicValue"] = 1.0f;
blue.floatOverrides["u_RoughnessValue"] = 0.2f;

// Green rough cube
MaterialOverridesComponent green;
green.vec3Overrides["u_AlbedoColor"] = glm::vec3(0.0f, 1.0f, 0.0f);
green.floatOverrides["u_MetallicValue"] = 0.0f;
green.floatOverrides["u_RoughnessValue"] = 1.0f;
```

### Test 2: Edit in Inspector
1. Select an entity with MaterialOverridesComponent in the hierarchy
2. Expand MaterialOverridesComponent in the inspector
3. Expand "floatOverrides" to see/edit metallic and roughness values
4. Expand "vec3Overrides" to see/edit colors with color picker
5. Changes should apply immediately when you edit values

### Test 3: Verify Instancing Still Works
- Create 100 cubes with the same material (no overrides) - should use instancing
- Add MaterialOverridesComponent to ONE cube - only that one should break instancing
- The other 99 should still be instanced together

## Current Implementation Status

✅ Component structure defined
✅ Reflection registration complete
✅ Inspector support for viewing/editing maps
✅ Serialization support (via reflection)
✅ MaterialOverridesSystem runtime application (uses MaterialPropertyBlock)
✅ GPU instancing preservation with overrides (property blocks don't break instancing!)

## Inspector Features

The inspector now supports:
- **floatOverrides**: Shows as InputFloat widgets
- **vec3Overrides**: Shows as ColorEdit3 widgets (with color picker!)
- **vec4Overrides**: Shows as ColorEdit4 widgets
- **textureOverrides**: Shows as text (read-only for now)
- **mat4Overrides**: Not yet implemented

Empty maps show "(empty)" text.

## Troubleshooting

**Q: My overrides aren't applying to the rendered mesh**
A: Check that MaterialOverridesSystem is initialized in Engine::Init(). Look for log message: "MaterialOverridesSystem: Initialization complete"

**Q: Inspector shows MaterialOverridesComponent but it's empty**
A: This is normal if you haven't added any overrides yet. Use the code examples above to populate the maps.

**Q: Changes in inspector don't apply**
A: Make sure the property names match your shader uniforms exactly (case-sensitive).

**Q: All my cubes have different colors but instancing broke**
A: This is expected if every cube has unique overrides. For instancing to work, entities must share the same material AND same override values (or no overrides).
