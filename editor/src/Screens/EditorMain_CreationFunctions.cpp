#include "Screens\EditorMain.hpp"

#include "component/Identification.hpp"
#include "Component/MaterialOverridesComponent.hpp"
#include "Physics/Physics_System.h"
#include "Render/Render.h"

void EditorMain::CreateDefaultEntity()
{
	// FIXED: Execute on engine thread
	engineService.ExecuteOnEngineThread([]() {
		ecs::world world = Engine::GetWorld();

		// Create new entity
		auto entity = world.add_entity();

		// Add basic components
		world.add_component_to_entity<TransformComponent>(entity);
		world.add_component_to_entity<VisibilityComponent>(entity, true);
		});
}

void EditorMain::CreatePlaneEntity()
{
	// FIXED: Execute on engine thread
	engineService.ExecuteOnEngineThread([]() {
		ecs::world world = Engine::GetWorld();

		// Create new entity
		auto entity = world.add_entity();

		// Add transform components
		world.add_component_to_entity<TransformComponent>(entity, TransformComponent{ glm::vec3{ 1,1,1 }, glm::vec3{}, glm::vec3(0.0f, 0.0f, 0.0f) });
		world.add_component_to_entity<VisibilityComponent>(entity, true);


		// Add mesh renderer component
		MeshRendererComponent meshRenderer;
		meshRenderer.isPrimitive = true;
		meshRenderer.m_PrimitiveType = MeshRendererComponent::PrimitiveType::PLANE;
		meshRenderer.hasAttachedMaterial = false;
		meshRenderer.material.m_AlbedoColor = glm::vec3(0.8f, 0.3f, 0.3f);
		meshRenderer.material.metallic = 0.1f;
		meshRenderer.material.roughness = 0.8f;
		meshRenderer.material.m_MaterialGuid.m_guid = rp::null_guid; // Use 0 for default material

		world.add_component_to_entity<MeshRendererComponent>(entity, meshRenderer);

		// Add material overrides for customization (replaces embedded struct)
		MaterialOverridesComponent materialOverrides;
		materialOverrides.vec3Overrides["u_AlbedoColor"] = glm::vec3(0.8f, 0.3f, 0.3f); // Reddish color
		materialOverrides.floatOverrides["u_MetallicValue"] = 0.0f; // Non-metallic (dielectric)
		materialOverrides.floatOverrides["u_RoughnessValue"] = 0.9f; // Very rough (matte ground)
		world.add_component_to_entity<MaterialOverridesComponent>(entity, materialOverrides);
		});
}

void EditorMain::CreateLightEntity()
{
	// FIXED: Execute on engine thread
	engineService.ExecuteOnEngineThread([]() {
		ecs::world world = Engine::GetWorld();

		// Create new entity
		auto entity = world.add_entity();

		// Add position
		world.add_component_to_entity<TransformComponent>(entity, TransformComponent{ glm::vec3{ 1,1,1 }, glm::vec3{}, glm::vec3(0.0f, 5.0f, 0.0f) });

		// Add light component (default initializers handle most fields)
		LightComponent light;
		light.m_Type = Light::Type::Directional;
		light.m_Direction = glm::vec3(0.0f, -1.0f, 0.0f);
		light.m_Color = glm::vec3(1.0f, 1.0f, 1.0f);
		light.m_IsEnabled = true;
		light.m_Intensity = 1.f;
		light.m_Range = 10.0f;
		light.m_InnerCone = 30.0f;
		light.m_OuterCone = 45.0f;

		world.add_component_to_entity<LightComponent>(entity, light);
		});
}

void EditorMain::CreateCameraEntity()
{
	// FIXED: Execute on engine thread
	engineService.ExecuteOnEngineThread([]() {
		ecs::world world = Engine::GetWorld();

		// Create new entity
		auto entity = world.add_entity();

		// Add position component
		world.add_component_to_entity<TransformComponent>(entity, TransformComponent{ glm::vec3{ 1,1,1 }, glm::vec3{}, glm::vec3(0.0f, 2.0f, 5.0f) });

		// Add camera component
		CameraComponent camera;
		camera.m_Type = CameraComponent::CameraType::PERSPECTIVE;
		camera.m_IsActive = true;
		camera.m_Fov = 45.0f;
		camera.m_AspectRatio = 16.0f / 9.0f;
		camera.m_Near = 0.1f;
		camera.m_Far = 100.0f;
		camera.m_Up = glm::vec3(0.0f, 1.0f, 0.0f);
		camera.m_Right = glm::vec3(1.0f, 0.0f, 0.0f);
		camera.m_Front = glm::vec3(0.0f, 0.0f, -1.0f);
		camera.m_Yaw = -90.0f;  // Initially looking down -Z axis
		camera.m_Pitch = 0.0f;

		world.add_component_to_entity<CameraComponent>(entity, camera);
		});
}

void EditorMain::CreateDemoScene()
{
	spdlog::info("Creating demo scene with cubes and lighting");

	// Create 3x3 cube grid using local utilities
	CreateCubeGrid(3, 3.0f);

	// FIXED: Create light entity on engine thread
	engineService.ExecuteOnEngineThread([]() {
		ecs::world world = Engine::GetWorld();

		auto lightEntity = world.add_entity();
		world.add_component_to_entity<TransformComponent>(lightEntity, TransformComponent{ glm::vec3{1.f}, glm::vec3{}, glm::vec3(0.0f, 5.0f, 0.0f) });

		LightComponent light;
		light.m_Type = Light::Type::Directional;
		light.m_Direction = glm::vec3(0.2f, -0.8f, 0.3f);
		light.m_Color = glm::vec3(1.0f, 0.95f, 0.85f);
		light.m_Intensity = 1.0f;
		light.m_IsEnabled = true;
		light.m_Range = 10.0f;
		light.m_InnerCone = 30.0f;
		light.m_OuterCone = 45.0f;

		world.add_component_to_entity<LightComponent>(lightEntity, light);
		});

	// Set stronger ambient light for better visibility
	engineService.SetAmbientLight(glm::vec3(0.03f));

	spdlog::info("Demo scene created with 9 cubes and enhanced lighting");

	//CreatePhysicsDemoScene();

}

void EditorMain::CreatePhysicsCube()
{
	// FIXED: Execute on engine thread
	engineService.ExecuteOnEngineThread([this]() {
		ecs::world world = Engine::GetWorld();

		// Cube creation
		auto CPos = glm::vec3(1.0f, 22.0f, 0.0f);
		auto Cscale = glm::vec3(1.0f);
		auto Ccolor = glm::vec3(1.0f, 0.0f, 0.0f);
		auto entity2 = world.add_entity();

		// Add transform components
		world.add_component_to_entity<TransformComponent>(entity2, TransformComponent{ Cscale, glm::vec3{}, CPos });
		world.add_component_to_entity<VisibilityComponent>(entity2, true);
		world.add_component_to_entity<RigidBodyComponent>(entity2);

		// Use shared primitive mesh and default material
		rp::Guid meshGuid2{}; // Use 0 for primitive shared meshes
		rp::Guid materialGuid2{}; // Use 0 for default material

		// Add mesh renderer component
		MeshRendererComponent meshRenderer2;
		meshRenderer2.isPrimitive = true; // Mark as primitive for proper handling
		meshRenderer2.m_PrimitiveType = MeshRendererComponent::PrimitiveType::CUBE;
		meshRenderer2.m_MeshGuid.m_guid = meshGuid2;
		meshRenderer2.hasAttachedMaterial = false;
		meshRenderer2.m_MaterialGuid["unnamed slot"].m_guid = materialGuid2;

		world.add_component_to_entity<MeshRendererComponent>(entity2, meshRenderer2);

		// Add material overrides
		MaterialOverridesComponent materialOverrides2;
		materialOverrides2.vec3Overrides["u_AlbedoColor"] = Ccolor; // Use color directly (no multiplier to avoid clamping)
		materialOverrides2.floatOverrides["u_MetallicValue"] = 0.0f; // Non-metallic (dielectric materials like plastic/wood)
		materialOverrides2.floatOverrides["u_RoughnessValue"] = 0.7f; // Slightly rough for diffuse appearance
		world.add_component_to_entity<MaterialOverridesComponent>(entity2, materialOverrides2);
		auto RigidBody = &world.get_component_from_entity<RigidBodyComponent>(entity2);
		// Creating Cube
		entity2.add<BoxCollider>();
		}); // End of ExecuteOnEngineThread lambda
}

void EditorMain::CreateCube(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& color)
{
	assert(scale.x > 0 && scale.y > 0 && scale.z > 0 && "Scale must be positive");

	// FIXED: Execute on engine thread
	engineService.ExecuteOnEngineThread([position, scale, color]() {
		auto world = Engine::GetWorld();
		auto entity = world.add_entity();

		// Add transform components
		world.add_component_to_entity<TransformComponent>(entity, TransformComponent{ scale, glm::vec3{}, position });

		world.add_component_to_entity<VisibilityComponent>(entity, true);

		// Use shared primitive mesh and default material
		rp::Guid meshGuid{}; // Use 0 for primitive shared meshes
		rp::Guid materialGuid{}; // Use 0 for default material

		// Add mesh renderer component
		MeshRendererComponent meshRenderer;
		meshRenderer.isPrimitive = true; // Mark as primitive for proper handling
		meshRenderer.m_PrimitiveType = MeshRendererComponent::PrimitiveType::CUBE;
		meshRenderer.m_MeshGuid.m_guid = meshGuid;
		meshRenderer.hasAttachedMaterial = false;
		meshRenderer.m_MaterialGuid["unnamed slot"].m_guid = materialGuid;

		world.add_component_to_entity<MeshRendererComponent>(entity, meshRenderer);

		// Add material overrides
		MaterialOverridesComponent materialOverrides;
		materialOverrides.vec3Overrides["u_AlbedoColor"] = color; // Use color directly (no multiplier to avoid clamping)
		materialOverrides.floatOverrides["u_MetallicValue"] = 0.0f; // Non-metallic (dielectric materials like plastic/wood)
		materialOverrides.floatOverrides["u_RoughnessValue"] = 0.7f; // Slightly rough for diffuse appearance
		world.add_component_to_entity<MaterialOverridesComponent>(entity, materialOverrides);
		});
}

void EditorMain::CreateCubeGrid(int gridSize, float spacing)
{
	assert(gridSize > 0 && gridSize <= 10 && "Grid size must be between 1-10");
	assert(spacing > 0.0f && "Spacing must be positive");

	// Vibrant colors for easy visual distinction
	std::vector<glm::vec3> colors = {
		glm::vec3(0.9f, 0.1f, 0.1f),  // Red - vibrant red
		glm::vec3(0.1f, 0.9f, 0.1f),  // Green - vibrant green
		glm::vec3(0.1f, 0.4f, 0.9f),  // Blue - vibrant blue
		glm::vec3(0.95f, 0.75f, 0.05f),  // Gold - bright gold
		glm::vec3(0.9f, 0.9f, 0.9f),  // Light gray - easier to see than white
	};

	const float startOffset = -(gridSize - 1) * spacing * 0.5f;

	for (int x = 0; x < gridSize; ++x) {
		for (int z = 0; z < gridSize; ++z) {
			glm::vec3 position(
				startOffset + x * spacing,
				0.0f,  // Ground level
				startOffset + z * spacing
			);

			// Cycle through materials based on position
			int colorIndex = (x + z) % colors.size();
			glm::vec3 color = colors[colorIndex];

			CreateCube(position, glm::vec3(1.0f), color);
		}
	}
}
