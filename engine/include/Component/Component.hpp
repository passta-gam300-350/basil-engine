/******************************************************************************/
/*!
\file   Component.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Base component class declaration

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef COMPONENT_HPP
#define COMPONENT_HPP
#include <ecs/ecs.h>

#include "Handle.hpp"

#ifndef NO_DEF_UUID
#include "uuid/uuid.hpp"
#endif

class Component {
protected:
	ComponentHandle handle;
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