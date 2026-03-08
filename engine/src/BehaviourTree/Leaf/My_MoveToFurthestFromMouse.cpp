// Name: Saminathan Aaron Nicholas
// DigiPen ID: s.aaronnicholas
// Class: CS380
// Date: 22 May 2025

#include <pch.h>
#include "My_MoveToFurthestFromMouse.h"
#include "Agent/BehaviorAgent.h"

void My_MoveToFurthestFromMouse::on_enter()
{
    float longestDistance = std::numeric_limits<float>().min();
    Vec3 furthestPoint;
    bool targetFound = false;
    const auto &allAgents = agents->get_all_agents();

    const auto& bb = agent->get_blackboard();
    const auto& currPos = bb.get_value<Vec3>("Click Position");

    for (const auto & a : allAgents)
    {
        if (a != agent)
        {
            const auto &agentPos = a->get_position();
            const float distance = Vec3::Distance(currPos, agentPos);

            if (distance > longestDistance)
            {
                longestDistance = distance;
                furthestPoint = agentPos;
                targetFound = true;
            }
        }
    }

    if (targetFound == true)
    {
        targetPoint = furthestPoint;
		BehaviorNode::on_leaf_enter();
    }
    else on_failure();
}

void My_MoveToFurthestFromMouse::on_update(float dt)
{
    const auto result = agent->move_toward_point(targetPoint, dt);

    if (result == true)
        on_success();

    display_leaf_text();
}

