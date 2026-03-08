// Name: Saminathan Aaron Nicholas
// DigiPen ID: s.aaronnicholas
// Class: CS380
// Date: 22 May 2025

#pragma once
#include "BehaviorNode.h"

class My_Idle : public BaseNode<My_Idle>
{
public:
    My_Idle();

protected:
    float timer;

    virtual void on_enter() override;
    virtual void on_update(float dt) override;
};