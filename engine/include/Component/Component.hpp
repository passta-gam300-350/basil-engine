#ifndef COMPONENT_HPP
#define COMPONENT_HPP
#include <ecs/ecs.h>

#include "Handle.hpp"

#ifndef NO_DEF_UUID
#include "uuid/uuid.hpp"
#endif

class Component {
protected:
	Handle handle;
public:
	bool enabled = true;
	Component() = default;

    enum class ComponentType : char
    {
		#include "Component.def"
		UNKNOWN
    };

	virtual bool inEditor();
	virtual ComponentType getType() const = 0;
	virtual ~Component() = default;


	virtual void Enable() { enabled = true; }
	virtual void Disable() { enabled = false; }
	bool isEnabled() const { return enabled; }
};

#endif //!COMPONENT_HPP