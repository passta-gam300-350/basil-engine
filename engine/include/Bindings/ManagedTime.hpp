#ifndef MANAGED_TIME_HPP
#define MANAGED_TIME_HPP
#include "Engine.hpp"

class ManagedTime
{
public:
	static float GetDeltaTime()
	{
		return static_cast<float>(Engine::GetDeltaTime());
	}
};
#endif// MANAGED_TIME_HPP
