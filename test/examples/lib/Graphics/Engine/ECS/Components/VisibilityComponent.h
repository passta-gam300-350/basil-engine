#pragma once

// Pure ECS component - stores visibility state
// No rendering concerns, just data
struct VisibilityComponent
{
    bool IsVisible = true;           // Is this entity currently visible?
    bool WasCulled = false;         // Was this entity culled in last frame?
    float BoundingSphereRadius = 1.0f; // For culling calculations
    
    // Default constructor
    VisibilityComponent() = default;
    
    VisibilityComponent(bool visible, float radius = 1.0f)
        : IsVisible(visible), BoundingSphereRadius(radius)
    {
    }
};