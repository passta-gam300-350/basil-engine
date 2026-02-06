// Name: Saminathan Aaron Nicholas
// DigiPen ID: s.aaronnicholas
// Class: CS380
// Date: 22 May 2025

#pragma once
#include "BehaviorNode.h"

class My_Wait : public BaseNode<My_Wait>
{
public:
    My_Wait();

protected:
    float timer;

    virtual void on_enter() override;
    virtual void on_update(float dt) override;
};