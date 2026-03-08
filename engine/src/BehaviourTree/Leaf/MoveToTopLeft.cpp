// Name: Saminathan Aaron Nicholas
// DigiPen ID: s.aaronnicholas
// Module: CSD 3451 - Software Engineering Project 6
// Date: 25 Jan 2026
#include <pch.h>
#include "L_MoveToTopLeft.h"
#include "Agent/BehaviorAgent.h"

void L_MoveToTopLeft::on_enter()
{
    Vec2 min = { 0.0f, 0.0f };

    int mapHeight = Terrain::get_map_height();
    int mapWidth = Terrain::get_map_width();

    Vec2 max = { static_cast<float>(mapWidth), static_cast<float>(mapHeight) };

    targetPoint = Vec2(min.x, max.y);
    BehaviorNode::on_leaf_enter();
}

void L_MoveToTopLeft::on_update(float dt)
{
    if (agent->move_toward_point(targetPoint, dt))
        on_success();

    display_leaf_text();
}
