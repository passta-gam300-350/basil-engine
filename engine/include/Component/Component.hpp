#ifndef COMPONENT_HPP
#define COMPONENT_HPP
#include <entt/entt.hpp>

class Component {
protected:

	entt::entity m_Entity;

public:
    enum class ComponentType : char
    {
	    TYPE_A,
		TYPE_B,
    };
	
	virtual ComponentType getType() const = 0;
	virtual ~Component() = default;
};
