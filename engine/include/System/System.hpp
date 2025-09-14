#ifndef SYSTEM_HPP
#define SYSTEM_HPP

namespace ecs
{
	struct world;
}

class System
{
	virtual void Init() = 0;
	virtual void Update(ecs::world&) = 0;
	virtual void FixedUpdate(ecs::world&) = 0;
	virtual void Exit() = 0;
	virtual ~System() = default;
};


#endif //!SYSTEM_HPP