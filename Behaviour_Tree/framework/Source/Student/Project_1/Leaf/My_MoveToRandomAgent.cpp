// Name: Saminathan Aaron Nicholas
// DigiPen ID: s.aaronnicholas
// Class: CS380
// Date: 23 May 2025

#include <pch.h>
#include "My_MoveToRandomAgent.h"
#include "Agent/BehaviorAgent.h"

void My_MoveToRandomAgent::on_enter()
{
    int typeInt = 0;
    typeInt = RNG::range(0, 3);

    const char* type = "";
    switch (typeInt)
    {
    case 0:
        type = "Man";
        break;
    case 1:
        type = "Car";
        break;
    case 2:
        type = "Bird";
        break;
    case 3:
        type = "Tree";
        break;
    }

    if (type != agent->get_type())
    {
        const auto& agent = agents->get_all_agents_by_type(type);

        targetPoint = agent[0]->get_position();
        BehaviorNode::on_leaf_enter();
    }
    else on_failure();
}

void My_MoveToRandomAgent::on_update(float dt)
{
    const auto res = agent->move_toward_point(targetPoint, dt);

    if (res == true)
        on_success();

    display_leaf_text();
}

