# Graphics Library - Standalone Usage Example

The graphics library has been refactored to be completely standalone and can now be used with any application architecture (ECS, simple arrays, or custom data structures).

## Key Features

- **Data Submission API**: Applications push rendering data to the graphics library each frame
- **No ECS Dependencies**: Works with any data storage approach
- **Pipeline Control**: Hybrid approach with sensible defaults and customization options
- **Efficient Rendering**: Built-in frustum culling, instancing, and PBR lighting

## Basic Usage

```cpp
#include <Graphics/Scene/SceneRenderer.h>
#include <Graphics/Utility/Camera.h>
#include <Graphics/Utility/RenderData.h>

// Create the main graphics renderer
SceneRenderer sceneRenderer;

// Create camera
auto camera = std::make_shared<Camera>();
camera->SetPosition(glm::vec3(0.0f, 0.0f, 5.0f));
sceneRenderer.SetCamera(camera);

// Main render loop
while (applicationRunning) {
    // Clear previous frame data
    sceneRenderer.ClearFrame();
    
    // Submit renderable objects
    for (auto& gameObject : myGameObjects) {
        RenderableData renderable;
        renderable.mesh = gameObject.mesh;
        renderable.material = gameObject.material;
        renderable.transform = gameObject.transform;
        renderable.visible = gameObject.visible;
        
        sceneRenderer.SubmitRenderable(renderable);
    }
    
    // Submit lights
    for (auto& light : myLights) {
        SubmittedLightData lightData;
        lightData.type = light.type;
        lightData.position = light.position;
        lightData.color = light.color;
        lightData.intensity = light.intensity;
        lightData.enabled = light.enabled;
        
        sceneRenderer.SubmitLight(lightData);
    }
    
    // Set ambient lighting
    sceneRenderer.SetAmbientLight(glm::vec3(0.1f, 0.1f, 0.1f));
    
    // Render everything
    sceneRenderer.Render();
}
```

## Using with Different Data Storage Systems

### With EnTT ECS

```cpp
void RenderWithEnTT(entt::registry& registry, SceneRenderer& renderer) {
    renderer.ClearFrame();
    
    auto view = registry.view<MeshComponent, TransformComponent, MaterialComponent>();
    for (auto entity : view) {
        auto& mesh = view.get<MeshComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        auto& material = view.get<MaterialComponent>(entity);
        
        RenderableData renderable;
        renderable.mesh = mesh.mesh;
        renderable.material = material.material;
        renderable.transform = transform.GetTransform();
        renderable.visible = true;
        
        renderer.SubmitRenderable(renderable);
    }
    
    renderer.Render();
}
```

### With Simple Arrays

```cpp
struct GameObject {
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;
    glm::mat4 transform;
    bool visible = true;
};

void RenderWithArrays(std::vector<GameObject>& objects, SceneRenderer& renderer) {
    renderer.ClearFrame();
    
    for (const auto& obj : objects) {
        RenderableData renderable;
        renderable.mesh = obj.mesh;
        renderable.material = obj.material;
        renderable.transform = obj.transform;
        renderable.visible = obj.visible;
        
        renderer.SubmitRenderable(renderable);
    }
    
    renderer.Render();
}
```

## Advanced Pipeline Control

```cpp
// Get access to specific pipelines for customization
auto* mainPipeline = sceneRenderer.GetPipeline("MainRendering");

// Disable shadow rendering
sceneRenderer.EnablePipeline("ShadowPass", false);

// Add custom post-processing pipeline
auto customPipeline = std::make_unique<RenderPipeline>("CustomPostProcess");
// ... configure custom pipeline ...
sceneRenderer.AddPipeline("CustomPostProcess", std::move(customPipeline));

// Set pipeline execution order
sceneRenderer.SetPipelineOrder({"ShadowPass", "MainRendering", "CustomPostProcess"});
```

## Benefits

1. **Framework Agnostic**: Works with any application architecture
2. **No Data Copying**: Efficient data submission without unnecessary copies
3. **Clean Separation**: Graphics logic separated from game/application logic
4. **Flexible**: Can be used for simple graphics apps or complex game engines
5. **Efficient**: Built-in optimizations like frustum culling and instanced rendering