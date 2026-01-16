// Name: Saminathan Aaron Nicholas
// DigiPen ID: s.aaronnicholas
// Class: CS380
// Date: 23 May 2025

#include <pch.h>
#include "My_MoveToTree.h"
#include "Agent/BehaviorAgent.h"

void My_MoveToTree::on_enter()
{
    const auto &treeAgent = agents->get_all_agents_by_type("Tree");

    targetPoint = treeAgent[0]->get_position();
	BehaviorNode::on_leaf_enter();
}

void My_MoveToTree::on_update(float dt)
{
    const auto res = agent->move_toward_point(targetPoint, dt);

    if (res == true)
        on_success();

    display_leaf_text();
}

