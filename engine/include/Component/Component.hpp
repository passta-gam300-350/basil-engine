#ifndef COMPONENT_HPP
#define COMPONENT_HPP
#include <ecs/ecs.h>

#include "Handle.hpp"
#include "uuid/uuid.hpp"

class Component {
protected:
	Handle handle;
public:
	Component() = default;

    enum class ComponentType : char
    {
		#include "Component.def"
		UNKNOWN
    };

	virtual bool inEditor() = 0;
	virtual ComponentType getType() const = 0;
	virtual ~Component() = default;
};

#endif //!COMPONENT_HPP