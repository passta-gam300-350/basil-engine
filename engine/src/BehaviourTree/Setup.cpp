// Name: Saminathan Aaron Nicholas
// DigiPen ID: s.aaronnicholas
// Class: CS380
// Date: 24 May 2025

#include <pch.h>
#include "Projects/ProjectOne.h"
#include "Agent/CameraAgent.h"

void ProjectOne::setup()
{
    // Create an agent (using the default "Agent::AgentModel::Man" model)
    auto man = agents->create_behavior_agent("Man", BehaviorTreeTypes::Man);

    // You can change properties here or at runtime from a behavior tree leaf node
    // Look in Agent.h for all of the setters, like these:
    man->set_color(Vec3(0.0f, 0.0f, 0.0f));
    // man->set_scaling(Vec3(7,7,7));
    // man->set_position(Vec3(100, 0, 100));

    // Create an agent with a different 3D model:
    // 1. (optional) Add a new 3D model to the framework other than the ones provided:
    //    A. Find a ".sdkmesh" model or use https://github.com/walbourn/contentexporter
    //       to convert fbx files (many end up corrupted in this process, so good luck!)
    //    B. Add a new AgentModel enum for your model in Agent.h (like the existing Man or Tree).
    // 2. Register the new model with the engine, so it associates the file path with the enum
    //    A. Here we are registering all of the extra models that already come in the package.
    //Agent::add_model("Assets\\tree.sdkmesh", Agent::AgentModel::Tree);


    // 3. Create the agent, giving it the correct AgentModel type.
    //auto car = agents->create_behavior_agent("Car", BehaviorTreeTypes::Car, Agent::AgentModel::Car);
    //auto tree = agents->create_behavior_agent("Tree", BehaviorTreeTypes::Tree, Agent::AgentModel::Tree);
    //auto bird = agents->create_behavior_agent("Bird", BehaviorTreeTypes::Bird, Agent::AgentModel::Bird);

    // 4. (optional) You can also set the pitch of the model, if you want it to be rotated differently
    // 5. (optional) Set other aspects to make it start out correctly

    //tree->set_pitch(PI / 2.0f);
    //tree->set_position(Vec3(terrain->mapSizeInWorld * 0.5f, 0.0f, terrain->mapSizeInWorld * 0.5f));
 
    //bird->set_scaling(Vec3(0.01f, 0.01f, 0.01f));
    //bird->set_position(Vec3(terrain->mapSizeInWorld * 0.5f, terrain->mapSizeInWorld * 0.0f, terrain->mapSizeInWorld * 1.0f));

    // You can technically load any map you want, even create your own map file,
    // but behavior agents won't actually avoid walls or anything special, unless you code
    // that yourself (that's the realm of project 2)
    terrain->goto_map(0);

    // You can also enable the pathing layer and set grid square colors as you see fit.
    // Works best with map 0, the completely blank map
    terrain->pathLayer.set_enabled(true);
    terrain->pathLayer.set_value(0, 0, Colors::Red);

    // Camera position can be modified from this default
    auto camera = agents->get_camera_agent();
    camera->set_position(Vec3(-62.0f, 70.0f, terrain->mapSizeInWorld * 0.5f));
    camera->set_pitch(0.610865); // 35 degrees

    // Sound control (these sound functions can be kicked off in a behavior tree node - see the example in L_PlaySound.cpp)
    audioManager->SetVolume(0.5f);
    audioManager->PlaySoundEffect(L"Assets\\Audio\\retro.wav");
    // Uncomment for example on playing music in the engine (must be .wav)
    // audioManager->PlayMusic(L"Assets\\Audio\\motivate.wav");
    // audioManager->PauseMusic(...);
    // audioManager->ResumeMusic(...);
    // audioManager->StopMusic(...);
}