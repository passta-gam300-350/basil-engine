// Name: Saminathan Aaron Nicholas
// DigiPen ID: s.aaronnicholas
// Class: CS380
// Date: 22 May 2025

#include <pch.h>
#include "My_Wait.h"

My_Wait::My_Wait() : timer(0.0f)
{}

void My_Wait::on_enter()
{
    timer = 1.0f;

	BehaviorNode::on_leaf_enter();
}

void My_Wait::on_update(float dt)
{
    timer -= dt;

    if (timer < 0.0f)
        on_success();

    display_leaf_text();
}
