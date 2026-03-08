// Name: Saminathan Aaron Nicholas
// DigiPen ID: s.aaronnicholas
// Class: CS380
// Date: 22 May 2025

#pragma once
#include "BehaviorNode.h"

class My_RepeatTwoTimes : public BaseNode<My_RepeatTwoTimes>
{
public:
    My_RepeatTwoTimes();

protected:
    unsigned counter;

    virtual void on_enter() override;
    virtual void on_update(float dt) override;
};