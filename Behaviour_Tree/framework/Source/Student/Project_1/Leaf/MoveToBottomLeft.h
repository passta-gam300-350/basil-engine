// Name: Saminathan Aaron Nicholas
// DigiPen ID: s.aaronnicholas
// Module: CSD 3451 - Software Engineering Project 6
// Date: 25 Jan 2026
#pragma once
#include "BehaviorNode.h"

class L_MoveToTopLeft : public BehaviorNode
{
public:
    void on_enter() override;
    void on_update(float dt) override;

private:
    Vec2 targetPoint;
};
