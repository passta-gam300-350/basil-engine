/******************************************************************************/
/*!
\file		Agent.h
\project	CS380/CS580 AI Framework
\author		Dustin Holmes
\summary	Base actor declarations

Copyright (C) 2018 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
*/
/******************************************************************************/

#pragma once
#include "../Misc/NiceTypes.h"

// forward declarations
class AgentOrganizer;

class Agent
{
    friend class AgentOrganizer;
public:
    Agent(const char *type, size_t id);

#pragma region Getters
    const Vec3 &get_position() const;
    const Vec3 &get_scaling() const;

    Vec3 get_forward_vector() const;
    Vec3 get_right_vector() const;
    Vec3 get_up_vector() const;

    // in radians
    float get_pitch() const;
    float get_yaw() const;
    float get_roll() const;

    const Vec3 &get_color() const;
    
    const Mat4 &get_local_to_world();

    const char *get_type() const;
    const size_t &get_id() const;

    float get_movement_speed() const;
#pragma endregion

#pragma region Setters
    void set_position(const Vec3 &pos);
    void set_scaling(const Vec3 &scale);
    void set_scaling(float scalar);

    void set_pitch(float angleRadians);
    void set_yaw(float angleRadians);
    void set_roll(float angleRadians);

    void set_color(const Vec3 &newColor);
    
    void set_movement_speed(float speed);
#pragma endregion

    virtual void update(float dt);

    enum class AgentModel
    {
        Man,
        Tree,
        Car,
        Bird,
        Ball,
        Hut,
    };

    // Additional: Getter and Setter for agentType
    AgentModel getAgentModel();
    void setAgentModel(AgentModel model);

    static void add_model(std::string modelPath, AgentModel model);
private:
    Vec3 position;
    Vec3 scaling;
    Vec3 eulerAngles;

    Mat4 localToWorld;
    bool isDirty;

    Vec3 color;
    
    const char *type;
    const size_t id;

    float movementSpeed;

    static  std::vector<std::unique_ptr<DirectX::Model>> Agent::models;
    static std::unordered_map<AgentModel, size_t> agentModelMap;
    void build_transformation();

    void draw_mesh(AgentModel model);
    virtual void draw_debug();
    static bool acquire_rendering_resources();
    static void release_rendering_resources();
    static void update_model(const Mat4 &view, const Mat4 &proj);


    // Additional variables
    AgentModel agentModel;
};