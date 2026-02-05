#include "Bindings/ManagedHudComponent.hpp"

#include "ecs/internal/entity.h"
#include "Render/Render.h"


void ManagedHudComponent::SetPosition(uint64_t handle, float x, float y)
{
	ecs::entity e{ handle };
	HUDComponent& cmp = e.get<HUDComponent>();	
	
	cmp.position = { x,y };

}

void ManagedHudComponent::GetPosition(uint64_t handle, float* out_x, float* out_y)
{
	ecs::entity e{ handle };
	HUDComponent& cmp = e.get<HUDComponent>();

	*out_x = cmp.position.x;
	*out_y = cmp.position.y;
}

bool ManagedHudComponent::GetVisibility(uint64_t handle)
{
	ecs::entity e{ handle };
	
	return e.any<HUDComponent>() ? (e.get<HUDComponent>().visible) : false;
	
}

void ManagedHudComponent::SetVisibility(uint64_t handle, bool visible)
{
	ecs::entity e{ handle };
	HUDComponent& cmp = e.get<HUDComponent>();
	cmp.visible = visible;
}
