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

ManagedAssembly::~ManagedAssembly() = default;