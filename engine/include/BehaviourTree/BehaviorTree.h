/******************************************************************************/
/*!
\file		BehaviorTree.h
\project	CS380/CS580 AI Framework
\author		Dustin Holmes
\co-author  Saminathan Aaron Nicholas fro CSD3451
\summary	Behavior tree declarations

Copyright (C) 2018 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
*/
/******************************************************************************/

#pragma once

// forward declaration
class BehaviorNode;
class BehaviorTreePrototype;

class BehaviorTree
{
    friend class BehaviorTreePrototype;
public:
    BehaviorTree();
    ~BehaviorTree();

    void update(float dt);

    const char *get_tree_name() const;
private:
    BehaviorNode *rootNode;
    const char *treeName;
};

/*
Create BehaviorAgent - Characters that use behaviour trees must be created as BehaviorAgent objects.

Example:
auto agent = agents->create_behavior_agent("Man", BehaviorTreeTypes::Man);

Parameters:
Character name
BehaviorTreeTypes enum value

-------------------------------------------------------------------------------
Create / Modify a Behaviour Tree file - .bht file

Example tree structure:

TREENAME(Man)
TREENODE(Selector,0)
TREENODE(Attack,1)
TREENODE(Chase,1)
TREENODE(Patrol,1)

depth 0 = root node
higher depth = children nodes

-------------------------------------------------------------------------------
Register nodes

- Nodes.def
- Nodeheaders.h

Node files are found in ControlFlow / Decorator / Leaf folders depending on the type of node

-------------------------------------------------------------------------------
Behaviour Tree execution

Every frame the agent runs:
BehaviorAgent::update()

which calls:
tree.update(dt)

which executes:
rootNode->tick(dt)
*/