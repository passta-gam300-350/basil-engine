# Framework Usage Guide

A complete guide for developers who want to use this graphics framework in their applications.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Basic Application Setup](#basic-application-setup)
3. [Rendering Patterns](#rendering-patterns)
4. [Resource Management](#resource-management)
5. [Advanced Features](#advanced-features)
6. [Performance Tips](#performance-tips)
7. [Common Use Cases](#common-use-cases)
8. [API Reference](#api-reference)

---

## Getting Started

### Quick Start Template

```cpp
#include "Application.h"

class MyGraphicsApp : public Application
{
public:
    MyGraphicsApp() : Application("My App", 1280, 720) {}

protected:
    void LoadResources() override {
        // Load your assets here
        LoadShader("basic", "assets/shaders/basic.vert", "assets/shaders/basic.frag");
        LoadModel("cube", "assets/models/cube.obj");
    }
    
    void Render() override {
        // Render your scene here
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -5));
        RenderModel("cube", "basic", transform);
    }
    
    void Update(float deltaTime) override {
        // Update your application logic here
    }
};

int main() {
    MyGraphicsApp app;
    return app.Run();
}
```

### Project Integration

**1. Include Headers**
```cpp
#include "Application.h"           // Base application framework
#include "Scene/SceneRenderer.h"   // Advanced scene rendering
#include "Resources/ResourceManager.h"  // Asset management
```

**2. Link Libraries**
```cmake
target_link_libraries(MyApp 
    GraphicsLib     # The main graphics library
    glfw           # Window management
    OpenGL::GL     # OpenGL
)
```

**3. Asset Structure**
```
YourProject/
├── assets/
│   ├── shaders/
│   │   ├── basic.vert
│   │   ├── basic.frag
│   │   ├── bindless.vert      # Optional: for bindless textures
│   │   └── bindless.frag
│   ├── models/
│   │   └── your_model.obj
│   └── textures/
│       └── your_texture.jpg
└── src/
    └── main.cpp
```

---

## Basic Application Setup

### Application Lifecycle

The framework provides a clean application lifecycle that handles all the complexity:

```cpp
class MyApp : public Application {
public:
    MyApp() : Application("Window Title", 1920, 1080) {
        // Optional: Configure application settings
        SetVSync(true);
        SetFullscreen(false);
    }

protected:
    // Called once during startup
    void LoadResources() override {
        // Load all your assets here
        LoadShader("material", "shaders/basic.vert", "shaders/basic.frag");
        LoadModel("player", "models/character.obj");
        LoadModel("environment", "models/level.obj");
        
        // Setup camera
        GetCamera().SetPosition(glm::vec3(0, 5, 10));
        GetCamera().SetTarget(glm::vec3(0, 0, 0));
    }
    
    // Called every frame for game logic
    void Update(float deltaTime) override {
        // Handle input
        if (IsKeyPressed(GLFW_KEY_W)) {
            m_PlayerPos.z -= 5.0f * deltaTime;
        }
        if (IsKeyPressed(GLFW_KEY_S)) {
            m_PlayerPos.z += 5.0f * deltaTime;
        }
        
        // Update camera to follow player
        GetCamera().SetPosition(m_PlayerPos + glm::vec3(0, 5, 10));
        GetCamera().SetTarget(m_PlayerPos);
    }
    
    // Called every frame for rendering
    void Render() override {
        // Render player
        glm::mat4 playerTransform = glm::translate(glm::mat4(1.0f), m_PlayerPos);
        RenderModel("player", "material", playerTransform);
        
        // Render environment
        glm::mat4 worldTransform = glm::mat4(1.0f);  // Identity matrix
        RenderModel("environment", "material", worldTransform);
    }
    
private:
    glm::vec3 m_PlayerPos = glm::vec3(0, 0, 0);
};
```

### Input Handling

```cpp
// In your Update() method:
void Update(float deltaTime) override {
    // Keyboard input
    if (IsKeyPressed(GLFW_KEY_SPACE)) {
        m_Player.Jump();
    }
    if (IsKeyHeld(GLFW_KEY_A)) {
        m_Player.MoveLeft(deltaTime);
    }
    
    // Mouse input
    if (IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        glm::vec2 mousePos = GetMousePosition();
        // Handle mouse click at mousePos
    }
    
    // Mouse movement (for camera control)
    glm::vec2 mouseDelta = GetMouseDelta();
    if (glm::length(mouseDelta) > 0) {
        m_CameraController.Rotate(mouseDelta.x, mouseDelta.y);
    }
}
```

### Camera Control

```cpp
class CameraController {
public:
    void Update(Camera& camera, float deltaTime) {
        // WASD movement
        glm::vec3 movement(0.0f);
        if (IsKeyHeld(GLFW_KEY_W)) movement += camera.GetForward();
        if (IsKeyHeld(GLFW_KEY_S)) movement -= camera.GetForward();
        if (IsKeyHeld(GLFW_KEY_A)) movement -= camera.GetRight();
        if (IsKeyHeld(GLFW_KEY_D)) movement += camera.GetRight();
        
        if (glm::length(movement) > 0) {
            movement = glm::normalize(movement) * m_MoveSpeed * deltaTime;
            camera.SetPosition(camera.GetPosition() + movement);
        }
        
        // Mouse look
        glm::vec2 mouseDelta = GetMouseDelta();
        if (glm::length(mouseDelta) > 0) {
            m_Yaw += mouseDelta.x * m_LookSensitivity;
            m_Pitch -= mouseDelta.y * m_LookSensitivity;
            
            // Clamp pitch to avoid gimbal lock
            m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);
            
            // Apply rotation to camera
            glm::vec3 direction;
            direction.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
            direction.y = sin(glm::radians(m_Pitch));
            direction.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
            
            camera.SetDirection(glm::normalize(direction));
        }
    }
    
private:
    float m_MoveSpeed = 5.0f;
    float m_LookSensitivity = 0.1f;
    float m_Yaw = -90.0f;   // Start facing forward
    float m_Pitch = 0.0f;
};
```

---

## Rendering Patterns

### Pattern 1: Simple Object Rendering

For basic scenes with individual objects:

```cpp
void Render() override {
    // Clear the screen
    // (This is done automatically by the framework)
    
    // Render individual objects
    for (const auto& object : m_SceneObjects) {
        glm::mat4 transform = object.GetTransform();
        RenderModel(object.GetModelName(), object.GetShaderName(), transform);
    }
}
```

### Pattern 2: GPU Instancing for Many Objects

For rendering hundreds/thousands of similar objects efficiently:

```cpp
class MyApp : public Application {
protected:
    void LoadResources() override {
        LoadShader("instanced", "shaders/instanced.vert", "shaders/instanced.frag");
        LoadModel("tree", "models/tree.obj");
        
        // Setup instancing
        SetupForestInstancing();
    }
    
    void SetupForestInstancing() {
        // Get the instanced renderer
        auto* instancedRenderer = GetSceneRenderer()->GetInstancedRenderer();
        
        // Create material for instanced trees
        auto shader = GetResourceManager().GetShader("instanced");
        auto material = std::make_shared<Material>(shader, "TreeMaterial");
        
        // Setup mesh for instancing
        auto model = GetResourceManager().GetModel("tree");
        auto mesh = model->GetMeshes()[0];  // Assume single mesh
        instancedRenderer->SetMeshData("forest_trees", mesh, material);
        
        // Generate tree instances
        GenerateForest();
    }
    
    void GenerateForest() {
        auto* instancedRenderer = GetSceneRenderer()->GetInstancedRenderer();
        instancedRenderer->BeginInstanceBatch();
        
        // Create 1000 trees in a forest pattern
        for (int x = -50; x < 50; x += 5) {
            for (int z = -50; z < 50; z += 5) {
                // Add some randomness
                float offsetX = ((rand() % 100) / 100.0f - 0.5f) * 3.0f;
                float offsetZ = ((rand() % 100) / 100.0f - 0.5f) * 3.0f;
                float scale = 0.8f + ((rand() % 100) / 100.0f) * 0.4f;  // 0.8 to 1.2
                
                InstancedRenderer::InstanceData instance;
                instance.modelMatrix = glm::scale(
                    glm::translate(glm::mat4(1.0f), 
                                   glm::vec3(x + offsetX, 0, z + offsetZ)), 
                    glm::vec3(scale));
                instance.color = glm::vec4(0.2f + (rand() % 100) / 500.0f,  // Slight green variation
                                          0.8f + (rand() % 100) / 500.0f, 
                                          0.2f, 1.0f);
                
                instancedRenderer->AddInstance("forest_trees", instance);
            }
        }
        
        instancedRenderer->EndInstanceBatch();
    }
    
    void Render() override {
        // Instanced objects are rendered automatically by the framework
        // You can still render individual objects alongside them:
        
        glm::mat4 playerTransform = glm::translate(glm::mat4(1.0f), m_PlayerPos);
        RenderModel("player", "basic", playerTransform);
    }
};
```

### Pattern 3: Dynamic Scene Management

For scenes where objects are created/destroyed dynamically:

```cpp
class DynamicSceneApp : public Application {
protected:
    void LoadResources() override {
        LoadShader("basic", "shaders/basic.vert", "shaders/basic.frag");
        LoadModel("projectile", "models/bullet.obj");
        LoadModel("enemy", "models/enemy.obj");
    }
    
    void Update(float deltaTime) override {
        // Spawn enemies periodically
        m_EnemySpawnTimer += deltaTime;
        if (m_EnemySpawnTimer >= 2.0f) {  // Every 2 seconds
            SpawnEnemy();
            m_EnemySpawnTimer = 0.0f;
        }
        
        // Update all game objects
        for (auto it = m_GameObjects.begin(); it != m_GameObjects.end();) {
            it->Update(deltaTime);
            
            // Remove dead objects
            if (it->ShouldRemove()) {
                it = m_GameObjects.erase(it);
            } else {
                ++it;
            }
        }
        
        // Handle shooting
        if (IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            FireProjectile();
        }
    }
    
    void Render() override {
        // Render all dynamic objects
        for (const auto& obj : m_GameObjects) {
            RenderModel(obj.GetModelName(), "basic", obj.GetTransform());
        }
    }
    
private:
    std::vector<GameObject> m_GameObjects;
    float m_EnemySpawnTimer = 0.0f;
    
    void SpawnEnemy() {
        GameObject enemy;
        enemy.SetModel("enemy");
        enemy.SetPosition(glm::vec3(
            (rand() % 100 - 50) / 5.0f,  // Random X: -10 to 10
            0,
            -20  // Spawn far away
        ));
        enemy.SetVelocity(glm::vec3(0, 0, 5.0f));  // Move toward player
        m_GameObjects.push_back(enemy);
    }
    
    void FireProjectile() {
        GameObject projectile;
        projectile.SetModel("projectile");
        projectile.SetPosition(GetCamera().GetPosition());
        projectile.SetVelocity(GetCamera().GetForward() * 20.0f);
        m_GameObjects.push_back(projectile);
    }
};
```

---

## Resource Management

### Loading Assets

```cpp
void LoadResources() override {
    // Load shaders with automatic bindless detection
    LoadShaderAuto("material");  // Loads bindless or basic based on GPU
    
    // Or manually specify shaders
    LoadShader("custom", "shaders/vertex.glsl", "shaders/fragment.glsl");
    
    // Load 3D models (supports .obj format)
    LoadModel("character", "models/character.obj");
    LoadModel("weapon", "models/sword.obj");
    LoadModel("environment", "models/level.obj");
    
    // Models with textures are loaded automatically
    // The .obj file references .mtl materials which reference texture files
}

// Helper method for automatic shader selection
void LoadShaderAuto(const std::string& name) {
    if (BindlessTextureHelper::IsBindlessAvailable()) {
        LoadShader(name, "assets/shaders/bindless.vert", "assets/shaders/bindless.frag");
        std::cout << "✓ Using bindless textures for " << name << std::endl;
    } else {
        LoadShader(name, "assets/shaders/basic.vert", "assets/shaders/basic.frag");
        std::cout << "✓ Using traditional textures for " << name << std::endl;
    }
}
```

### Resource Access

```cpp
void SomeFunction() {
    // Access loaded resources through the ResourceManager
    auto& resourceManager = GetResourceManager();
    
    // Get loaded shader
    auto shader = resourceManager.GetShader("material");
    if (shader) {
        // Use shader...
    }
    
    // Get loaded model
    auto model = resourceManager.GetModel("character");
    if (model) {
        auto meshes = model->GetMeshes();
        // Access individual meshes...
    }
}
```

### Memory Management

The framework uses RAII and smart pointers for automatic resource cleanup:

```cpp
// Resources are automatically cleaned up when the application shuts down
// You don't need to manually delete anything

// However, if you need to free resources during runtime:
void FreeUnusedResources() {
    auto& resourceManager = GetResourceManager();
    
    // Resources are automatically freed when no longer referenced
    // Just stop holding references to them:
    m_MyModel.reset();  // If you stored a shared_ptr
    
    // The ResourceManager caches resources, but you can clear the cache:
    // (Note: This is not implemented in current version but could be added)
    // resourceManager.ClearUnusedResources();
}
```

---

## Advanced Features

### Working with the ECS System

For complex applications, you can work directly with the ECS system:

```cpp
#include "Engine/Scene/Scene.h"
#include "Engine/ECS/Components/TransformComponent.h"

class AdvancedApp : public Application {
protected:
    void LoadResources() override {
        LoadShader("basic", "shaders/basic.vert", "shaders/basic.frag");
        LoadModel("npc", "models/character.obj");
        
        // Create ECS entities directly
        SetupECSScene();
    }
    
    void SetupECSScene() {
        auto scene = GetScene();  // Get the ECS scene
        
        // Create NPCs as ECS entities
        for (int i = 0; i < 10; ++i) {
            auto npcEntity = scene->CreateEntity();
            
            // Add transform component
            auto& transform = scene->AddComponent<TransformComponent>(npcEntity);
            transform.Translation = glm::vec3(i * 2.0f, 0, 0);
            transform.Scale = glm::vec3(1.0f);
            
            // Add graphics components
            auto mesh = GetResourceManager().GetModel("npc")->GetMeshes()[0];
            auto material = std::make_shared<Material>(
                GetResourceManager().GetShader("basic"), "NPCMaterial");
            
            scene->AddComponent<MeshComponent>(npcEntity, MeshComponent{mesh});
            scene->AddComponent<MaterialComponent>(npcEntity, MaterialComponent{material});
            
            // Add custom game components
            scene->AddComponent<NPCComponent>(npcEntity);
            scene->AddComponent<HealthComponent>(npcEntity, HealthComponent{100});
        }
    }
    
    void Update(float deltaTime) override {
        // Update ECS systems
        auto scene = GetScene();
        
        // Example: Update NPC AI
        scene->GetRegistry().view<NPCComponent, TransformComponent>()
            .each([deltaTime](auto entity, NPCComponent& npc, TransformComponent& transform) {
                // Update NPC behavior
                npc.Update(deltaTime);
                
                // Move NPC based on AI decision
                if (npc.ShouldMove()) {
                    transform.Translation += npc.GetMovementDirection() * deltaTime;
                }
            });
    }
    
    void Render() override {
        // ECS entities are automatically rendered by MeshRenderer
        // You just need to ensure they have MeshComponent and MaterialComponent
        
        // You can still render additional non-ECS objects:
        glm::mat4 skyboxTransform = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f));
        RenderModel("skybox", "skybox_shader", skyboxTransform);
    }
};
```

### Custom Shaders and Materials

```cpp
// Create custom materials with different properties
void SetupCustomMaterials() {
    auto basicShader = GetResourceManager().GetShader("basic");
    
    // Create different material variations
    auto metalMaterial = std::make_shared<Material>(basicShader, "Metal");
    metalMaterial->SetFloat("u_Metallic", 1.0f);
    metalMaterial->SetFloat("u_Roughness", 0.2f);
    metalMaterial->SetVec3("u_AlbedoColor", glm::vec3(0.7f, 0.7f, 0.8f));
    
    auto plasticMaterial = std::make_shared<Material>(basicShader, "Plastic");
    plasticMaterial->SetFloat("u_Metallic", 0.0f);
    plasticMaterial->SetFloat("u_Roughness", 0.8f);
    plasticMaterial->SetVec3("u_AlbedoColor", glm::vec3(0.8f, 0.2f, 0.2f));
    
    // Store materials for later use
    m_Materials["metal"] = metalMaterial;
    m_Materials["plastic"] = plasticMaterial;
}

// Use different materials
void Render() override {
    // Render metal objects
    for (const auto& metalObj : m_MetalObjects) {
        RenderModelWithMaterial("cube", m_Materials["metal"], metalObj.transform);
    }
    
    // Render plastic objects  
    for (const auto& plasticObj : m_PlasticObjects) {
        RenderModelWithMaterial("cube", m_Materials["plastic"], plasticObj.transform);
    }
}
```

### Performance Profiling

```cpp
void Update(float deltaTime) override {
    // Enable profiling to see where time is spent
    static bool profilingEnabled = false;
    if (IsKeyPressed(GLFW_KEY_F1)) {
        profilingEnabled = !profilingEnabled;
        GetRenderer().EnableProfiling(profilingEnabled);
    }
    
    if (IsKeyPressed(GLFW_KEY_F2)) {
        // Print performance statistics
        auto stats = GetRenderer().GetPerformanceStats();
        std::cout << "Frame time: " << stats.frameTime << "ms" << std::endl;
        std::cout << "Draw calls: " << stats.drawCalls << std::endl;
        std::cout << "Triangles: " << stats.triangleCount << std::endl;
        
        auto commandStats = GetRenderer().GetCommandBuffer().GetStats();
        std::cout << "Commands submitted: " << commandStats.commandCount << std::endl;
        std::cout << "Command buffer memory: " << commandStats.memoryUsage << " bytes" << std::endl;
    }
}
```

---

## Performance Tips

### 1. Batch Similar Objects

```cpp
// Bad: Many individual objects
for (const auto& tree : forest) {
    RenderModel("tree", "basic", tree.transform);  // 1000 draw calls
}

// Good: Use instancing for similar objects
auto* instancedRenderer = GetSceneRenderer()->GetInstancedRenderer();
instancedRenderer->SetMeshData("trees", treeMesh, treeMaterial);

instancedRenderer->BeginInstanceBatch();
for (const auto& tree : forest) {
    InstancedRenderer::InstanceData instance;
    instance.modelMatrix = tree.transform;
    instance.color = tree.color;
    instancedRenderer->AddInstance("trees", instance);
}
instancedRenderer->EndInstanceBatch();
// Result: 1 draw call for 1000 trees!
```

### 2. Minimize State Changes

```cpp
// Bad: Mixed materials cause state changes
RenderModel("obj1", "material_A", transform1);
RenderModel("obj2", "material_B", transform2);
RenderModel("obj3", "material_A", transform3);  // State change back to A

// Good: Group by material
std::sort(objects.begin(), objects.end(), [](const Object& a, const Object& b) {
    return a.materialName < b.materialName;
});

for (const auto& obj : objects) {
    RenderModel(obj.modelName, obj.materialName, obj.transform);
}
// The framework's command system automatically handles this optimization
```

### 3. Use Appropriate Culling

```cpp
// The framework includes automatic frustum culling
// But you can optimize further with manual culling for special cases

void Update(float deltaTime) override {
    glm::vec3 cameraPos = GetCamera().GetPosition();
    
    // Cull objects outside a certain range
    for (auto& obj : m_DistantObjects) {
        float distance = glm::distance(obj.position, cameraPos);
        obj.visible = (distance < MAX_RENDER_DISTANCE);
    }
}

void Render() override {
    // Only render visible objects
    for (const auto& obj : m_DistantObjects) {
        if (obj.visible) {
            RenderModel(obj.modelName, obj.materialName, obj.transform);
        }
    }
}
```

### 4. Optimize Resource Loading

```cpp
// Load resources asynchronously during gameplay
class StreamingApp : public Application {
protected:
    void LoadResources() override {
        // Load essential resources immediately
        LoadShader("basic", "shaders/basic.vert", "shaders/basic.frag");
        LoadModel("player", "models/player.obj");
        
        // Start loading additional resources in background
        StartAsyncLoading();
    }
    
    void StartAsyncLoading() {
        // Use std::async for background loading
        m_AsyncLoaders.emplace_back(
            std::async(std::launch::async, [this]() {
                LoadModel("level_2", "models/level_2.obj");
                m_Level2Ready = true;
            })
        );
    }
    
    void Update(float deltaTime) override {
        // Check if async loading completed
        if (m_Level2Ready && m_ShouldLoadLevel2) {
            // Level 2 is ready to use
            SetupLevel2();
            m_ShouldLoadLevel2 = false;
        }
    }
    
private:
    std::vector<std::future<void>> m_AsyncLoaders;
    std::atomic<bool> m_Level2Ready{false};
    bool m_ShouldLoadLevel2 = false;
};
```

---

## Common Use Cases

### 1. Game Development

```cpp
class GameApp : public Application {
protected:
    void LoadResources() override {
        // Load game assets
        LoadShaderAuto("character");
        LoadShaderAuto("environment");
        LoadShaderAuto("effects");
        
        LoadModel("player", "models/hero.obj");
        LoadModel("enemy", "models/orc.obj");
        LoadModel("weapon", "models/sword.obj");
        LoadModel("level", "models/dungeon.obj");
        
        // Setup particle effects with instancing
        SetupParticleSystem();
    }
    
    void SetupParticleSystem() {
        // Use instancing for particle effects
        auto* instancedRenderer = GetSceneRenderer()->GetInstancedRenderer();
        auto particleShader = GetResourceManager().GetShader("effects");
        auto particleMaterial = std::make_shared<Material>(particleShader, "Particles");
        
        instancedRenderer->SetMeshData("fire_particles", CreateQuadMesh(), particleMaterial);
    }
    
    void Update(float deltaTime) override {
        // Game logic
        m_Player.Update(deltaTime);
        
        for (auto& enemy : m_Enemies) {
            enemy.Update(deltaTime);
            enemy.AI_Update(m_Player.GetPosition(), deltaTime);
        }
        
        // Update particle system
        UpdateParticles(deltaTime);
        
        // Handle collisions
        CheckCollisions();
    }
    
    void Render() override {
        // Render level
        RenderModel("level", "environment", glm::mat4(1.0f));
        
        // Render player
        RenderModel("player", "character", m_Player.GetTransform());
        
        // Render enemies
        for (const auto& enemy : m_Enemies) {
            RenderModel("enemy", "character", enemy.GetTransform());
        }
        
        // Particles are rendered automatically via instancing
    }
    
private:
    Player m_Player;
    std::vector<Enemy> m_Enemies;
    ParticleSystem m_ParticleSystem;
};
```

### 2. Data Visualization

```cpp
class DataVizApp : public Application {
protected:
    void LoadResources() override {
        LoadShader("data_point", "shaders/point.vert", "shaders/point.frag");
        LoadModel("sphere", "models/sphere.obj");
        
        // Load dataset
        LoadDataSet("data/scientific_data.csv");
        
        // Create instanced visualization
        SetupDataVisualization();
    }
    
    void LoadDataSet(const std::string& filename) {
        // Parse CSV data
        std::ifstream file(filename);
        std::string line;
        
        while (std::getline(file, line)) {
            DataPoint point = ParseCSVLine(line);
            m_DataPoints.push_back(point);
        }
        
        std::cout << "Loaded " << m_DataPoints.size() << " data points" << std::endl;
    }
    
    void SetupDataVisualization() {
        auto* instancedRenderer = GetSceneRenderer()->GetInstancedRenderer();
        auto shader = GetResourceManager().GetShader("data_point");
        auto material = std::make_shared<Material>(shader, "DataPoints");
        
        auto mesh = GetResourceManager().GetModel("sphere")->GetMeshes()[0];
        instancedRenderer->SetMeshData("data_viz", mesh, material);
        
        // Convert data points to visual instances
        instancedRenderer->BeginInstanceBatch();
        for (const auto& point : m_DataPoints) {
            InstancedRenderer::InstanceData instance;
            
            // Position based on data coordinates
            instance.modelMatrix = glm::scale(
                glm::translate(glm::mat4(1.0f), point.position),
                glm::vec3(point.size)
            );
            
            // Color based on data value
            instance.color = MapValueToColor(point.value);
            
            instancedRenderer->AddInstance("data_viz", instance);
        }
        instancedRenderer->EndInstanceBatch();
    }
    
private:
    struct DataPoint {
        glm::vec3 position;
        float value;
        float size;
    };
    
    std::vector<DataPoint> m_DataPoints;
    
    glm::vec4 MapValueToColor(float value) {
        // Map value to color gradient (blue to red)
        float normalized = (value - m_MinValue) / (m_MaxValue - m_MinValue);
        return glm::vec4(normalized, 0.0f, 1.0f - normalized, 1.0f);
    }
};
```

### 3. Architectural Visualization

```cpp
class ArchVizApp : public Application {
protected:
    void LoadResources() override {
        // Load high-quality shaders with PBR
        LoadShader("pbr", "shaders/pbr.vert", "shaders/pbr.frag");
        LoadShader("glass", "shaders/glass.vert", "shaders/glass.frag");
        
        // Load building components
        LoadModel("building_structure", "models/building.obj");
        LoadModel("furniture", "models/furniture.obj");
        LoadModel("vegetation", "models/plants.obj");
        
        // Setup different material zones
        SetupMaterials();
        
        // Use instancing for repetitive elements
        SetupInstancedElements();
    }
    
    void SetupMaterials() {
        auto pbrShader = GetResourceManager().GetShader("pbr");
        
        // Create different material types
        auto concreteMat = std::make_shared<Material>(pbrShader, "Concrete");
        concreteMat->SetFloat("u_Metallic", 0.0f);
        concreteMat->SetFloat("u_Roughness", 0.9f);
        concreteMat->SetVec3("u_AlbedoColor", glm::vec3(0.6f, 0.6f, 0.6f));
        
        auto woodMat = std::make_shared<Material>(pbrShader, "Wood");
        woodMat->SetFloat("u_Metallic", 0.0f);
        woodMat->SetFloat("u_Roughness", 0.7f);
        woodMat->SetVec3("u_AlbedoColor", glm::vec3(0.4f, 0.2f, 0.1f));
        
        auto metalMat = std::make_shared<Material>(pbrShader, "Steel");
        metalMat->SetFloat("u_Metallic", 1.0f);
        metalMat->SetFloat("u_Roughness", 0.3f);
        metalMat->SetVec3("u_AlbedoColor", glm::vec3(0.8f, 0.8f, 0.9f));
        
        m_Materials["concrete"] = concreteMat;
        m_Materials["wood"] = woodMat;
        m_Materials["steel"] = metalMat;
    }
    
    void SetupInstancedElements() {
        // Use instancing for repetitive architectural elements
        SetupInstancedWindows();
        SetupInstancedTrees();
        SetupInstancedStreetLights();
    }
    
    void Update(float deltaTime) override {
        // Update camera based on user input
        m_ArchVizCamera.Update(GetCamera(), deltaTime);
        
        // Update lighting based on time of day
        UpdateLighting(deltaTime);
        
        // Handle material switching for demonstration
        if (IsKeyPressed(GLFW_KEY_1)) m_CurrentMaterial = "concrete";
        if (IsKeyPressed(GLFW_KEY_2)) m_CurrentMaterial = "wood";
        if (IsKeyPressed(GLFW_KEY_3)) m_CurrentMaterial = "steel";
    }
    
    void Render() override {
        // Render main building structure
        RenderModelWithMaterial("building_structure", m_Materials[m_CurrentMaterial], 
                               glm::mat4(1.0f));
        
        // Render furniture
        for (const auto& furniture : m_FurnitureItems) {
            RenderModelWithMaterial("furniture", m_Materials["wood"], 
                                   furniture.transform);
        }
        
        // Instanced elements (windows, trees, lights) render automatically
    }
    
private:
    std::unordered_map<std::string, std::shared_ptr<Material>> m_Materials;
    std::string m_CurrentMaterial = "concrete";
    ArchVizCameraController m_ArchVizCamera;
    std::vector<FurnitureItem> m_FurnitureItems;
};
```

---

## API Reference

### Application Class

**Constructor**
```cpp
Application(const std::string& title, int width, int height)
```

**Virtual Methods to Override**
```cpp
virtual void LoadResources() {}      // Load assets here
virtual void Update(float deltaTime) {}  // Game logic here  
virtual void Render() {}             // Rendering here
virtual void OnWindowResize(int width, int height) {}  // Handle resize
```

**Resource Loading**
```cpp
void LoadShader(const std::string& name, const std::string& vertexPath, 
                const std::string& fragmentPath);
void LoadModel(const std::string& name, const std::string& filePath);
```

**Rendering**
```cpp
void RenderModel(const std::string& modelName, const std::string& shaderName, 
                 const glm::mat4& transform);
void RenderModelWithMaterial(const std::string& modelName, 
                            std::shared_ptr<Material> material,
                            const glm::mat4& transform);
```

**Input**
```cpp
bool IsKeyPressed(int key);          // Single press
bool IsKeyHeld(int key);             // Continuous hold
bool IsMouseButtonPressed(int button);
glm::vec2 GetMousePosition();
glm::vec2 GetMouseDelta();           // Movement since last frame
```

**Getters**
```cpp
ResourceManager& GetResourceManager();
Camera& GetCamera();
Scene* GetScene();                   // For ECS access
SceneRenderer* GetSceneRenderer();   // For advanced rendering features
Renderer& GetRenderer();             // For low-level access
```

### Camera Class

```cpp
void SetPosition(const glm::vec3& position);
void SetDirection(const glm::vec3& direction);
void SetTarget(const glm::vec3& target);

glm::vec3 GetPosition() const;
glm::vec3 GetForward() const;
glm::vec3 GetRight() const;
glm::vec3 GetUp() const;

glm::mat4 GetViewMatrix() const;
glm::mat4 GetProjectionMatrix() const;

void SetFOV(float fov);
void SetAspectRatio(float aspectRatio);
void SetNearFar(float near, float far);
```

### InstancedRenderer Class

```cpp
struct InstanceData {
    glm::mat4 modelMatrix;   // Transform matrix
    glm::vec4 color;         // Color tint
    uint32_t materialId;     // Material variation
    uint32_t flags;          // Custom flags
    float metallic;          // Metallic factor
    float roughness;         // Roughness factor
};

void SetMeshData(const std::string& meshId, std::shared_ptr<Mesh> mesh, 
                 std::shared_ptr<Material> material);

void BeginInstanceBatch();
void AddInstance(const std::string& meshId, const InstanceData& instance);
void EndInstanceBatch();  // Uploads to GPU

void ClearInstances(const std::string& meshId);
size_t GetInstanceCount(const std::string& meshId);
```

### Material Class

```cpp
void SetFloat(const std::string& name, float value);
void SetInt(const std::string& name, int value);
void SetVec3(const std::string& name, const glm::vec3& value);
void SetVec4(const std::string& name, const glm::vec4& value);
void SetMat4(const std::string& name, const glm::mat4& value);

float GetFloat(const std::string& name) const;
// ... other getters
```

This framework provides a clean, high-performance foundation for graphics applications. The command-based architecture ensures excellent performance while the high-level API keeps development simple and productive.