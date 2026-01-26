// Name: Saminathan Aaron Nicholas
// DigiPen ID: s.aaronnicholas
// Module: CSD 3451 - Software Engineering Project 6
// Date: 25 Jan 2026
#pragma once

// Include all node headers in this file
// Example Control Flow Nodes
#include "ControlFlow/C_ParallelSequencer.h"
#include "ControlFlow/C_RandomSelector.h"
#include "ControlFlow/C_Selector.h"
#include "ControlFlow/C_Sequencer.h"

// Example Decorator Nodes
#include "Decorator/D_Delay.h"
#include "Decorator/D_InvertedRepeater.h"
#include "Decorator/D_RepeatFourTimes.h"

// Student Decorator Nodes
#include "Decorator/My_RepeatTwoTimes.h"
#include "Decorator/My_RepeatThreeTimes.h"

// Example Leaf Nodes
#include "Leaf/L_CheckMouseClick.h"
#include "Leaf/L_Idle.h"
#include "Leaf/L_MoveToFurthestAgent.h"
#include "Leaf/L_MoveToMouseClick.h"
#include "Leaf/L_MoveToRandomPosition.h"
#include "Leaf/L_PlaySound.h"

// Student Leaf Nodes
#include "Leaf/My_Idle.h"
#include "Leaf/My_wait.h"
#include "Leaf/My_MoveToFurthestFromMouse.h"
#include "Leaf/My_MoveToMan.h"

// Movement Leaf Nodes
#include "Leaf/MoveToTopLeft.h"
#include "Leaf/MoveToBottomLeft.h"
#include "Leaf/MoveToTopRight.h"
#include "Leaf/My_MoveToBottomRight.h"