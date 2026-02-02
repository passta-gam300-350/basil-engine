// Name: Saminathan Aaron Nicholas
// DigiPen ID: s.aaronnicholas
// Class: CS380
// Date: 22 May 2025

#include <pch.h>
#include "My_RepeatThreeTimes.h"

My_RepeatThreeTimes::My_RepeatThreeTimes() : counter(0)
{}

void My_RepeatThreeTimes::on_enter()
{
    counter = 0;
    BehaviorNode::on_enter();
}

void My_RepeatThreeTimes::on_update(float dt)
{
    BehaviorNode *child = children.front();
    child->tick(dt);

    if (child->succeeded() == true)
    {
        ++counter;

        if (counter == 3)
            on_success();
        else child->set_status(NodeStatus::READY);
    }
    else if (child->failed() == true)
        on_failure();
}
