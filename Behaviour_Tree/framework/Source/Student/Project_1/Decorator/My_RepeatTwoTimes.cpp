// Name: Saminathan Aaron Nicholas
// DigiPen ID: s.aaronnicholas
// Class: CS380
// Date: 22 May 2025

#include <pch.h>
#include "My_RepeatTwoTimes.h"

My_RepeatTwoTimes::My_RepeatTwoTimes() : counter(0)
{}

void My_RepeatTwoTimes::on_enter()
{
    counter = 0;
    BehaviorNode::on_enter();
}

void My_RepeatTwoTimes::on_update(float dt)
{
    BehaviorNode *child = children.front();
    child->tick(dt);

    if (child->succeeded() == true)
    {
        ++counter;

        if (counter == 2)
            on_success();
        else child->set_status(NodeStatus::READY);
    }
    else if (child->failed() == true)
        on_failure();
}
