#include <pch.h>
#include "L_PlaySound.h"

void L_PlaySound::on_enter()
{
	audioManager->PlaySoundEffect(L"Assets\\Audio\\retro.wav");
	BehaviorNode::on_leaf_enter();
	on_success();
}