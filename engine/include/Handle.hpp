#ifndef HANDLE_HPP
#define HANDLE_HPP
#include <string>

#include "uuid/uuid.hpp"


struct Handle
{
	UUID<128> id;
	uint32_t handle_name;
	Handle() = default;
	Handle(std::string const& name);
	Handle(std::string const& name, UUID<128> const& uuid);
};

#endif // !HANDLE_HPP