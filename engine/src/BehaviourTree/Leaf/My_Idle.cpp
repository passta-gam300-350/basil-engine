// Name: Saminathan Aaron Nicholas
// DigiPen ID: s.aaronnicholas
// Class: CS380
// Date: 22 May 2025

#include <pch.h>
#include "My_Idle.h"

My_Idle::My_Idle() : timer(0.0f)
{}

void My_Idle::on_enter()
{
    timer = 3.0f;

	BehaviorNode::on_leaf_enter();
}

void My_Idle::on_update(float dt)
{
    timer -= dt;

    if (timer < 0.0f)
    {
        on_success();
    }

    display_leaf_text();
}
