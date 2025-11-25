#ifndef HANDLE_HPP
#define HANDLE_HPP
#include <string>

#include "uuid/uuid.hpp"

// Renamed from Handle to ComponentHandle to avoid conflict with Windows HANDLE and the ResourceSystem Handle struct
struct ComponentHandle
{
	uuid::UUID<128> id;
	uint32_t handle_name;
	ComponentHandle() = default;
	ComponentHandle(std::string name);
	ComponentHandle(std::string name, uuid::UUID<128> const& uuid);
};

#endif // !HANDLE_HPP