/******************************************************************************/
/*!
\file   ManagedAssembly.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Managed assembly implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "ABI/ManagedAssembly.hpp"
#include "ABI/CSAssembly.hpp"
#include <cassert>

ManagedAssembly::ManagedAssembly(std::unique_ptr<CSAssembly> assembly)
	: assembly(std::move(assembly)), assemblyHandle(nullptr), isLoaded(false)
{
	
}

bool ManagedAssembly::Load(MonoDomain* domain)
{
	if (isLoaded) {
		return true;
	}

	assert(!assemblyHandle && "Assembly handle should be null if not loaded.");
	assert(assembly && "Assembly pointer should not be null.");
		

	assemblyHandle = mono_domain_assembly_open(domain, assembly->Path().string().c_str());

	if (!assemblyHandle) {
		return false;
	}

	isLoaded = true;
	return true;
}

MonoImage* ManagedAssembly::Image() const noexcept
{
	if (!isLoaded || !assemblyHandle) {
		return nullptr;
	}
	return mono_assembly_get_image(assemblyHandle);
}
bool ManagedAssembly::IsLoaded() const noexcept
{
	return isLoaded;
}
std::string_view ManagedAssembly::Name() const noexcept
{
	if (assembly) {
		return assembly->Name();
	}
	return {};
}
std::string_view ManagedAssembly::Path() const noexcept
{
	if (assembly) {
		return assembly->Path().string();
	}
	return {};
}
std::size_t ManagedAssembly::Size() const noexcept
{
	if (assembly) {
		return assembly->Size();
	}
	return 0;
}
void ManagedAssembly::Unload() noexcept
{
	if (!isLoaded) {
		return;
	}
	assemblyHandle = nullptr;
	isLoaded = false;
}



ManagedAssembly::~ManagedAssembly() = default;