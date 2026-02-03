#pragma once
#include "BehaviorNode.h"

class L_PlaySound : public BaseNode<L_PlaySound>
{
protected:
    virtual void on_enter() override;
};