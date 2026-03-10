#include "Bindings/ManagedWorldUI.hpp"

#include "ecs/internal/entity.h"
#include "Render/Render.h"


void ManagedWorldUI::SetScale(uint64_t handle, float x, float y)
{
	ecs::entity e{ handle };
	if (!e.any<WorldUIComponent>()) return;

	auto& comp = e.get<WorldUIComponent>();
	comp.size.x = x;
	comp.size.y = y;
}

void ManagedWorldUI::GetScale(uint64_t handle, float* px, float* py)
{
	ecs::entity e{ handle };
	if (!e.any<WorldUIComponent>())
	{
		return;
	}
	auto& comp = e.get<WorldUIComponent>();
	*px = comp.size.x;
	*py = comp.size.y;

}

void ManagedWorldUI::SetVisibility(uint64_t handle, bool visible)
{
	ecs::entity e{ handle };
	if (!e.any<WorldUIComponent>()) return;
	auto& comp = e.get<WorldUIComponent>();
	comp.visible = visible;
}

bool ManagedWorldUI::GetVisibility(uint64_t handle)
{
	ecs::entity e{ handle };
	if (!e.any<WorldUIComponent>())
	{
		return false;
	}
	auto& comp = e.get<WorldUIComponent>();
	return comp.visible;
}

