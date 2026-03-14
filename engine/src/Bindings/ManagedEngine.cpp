#include "Bindings/ManagedEngine.hpp"

#include "Engine.hpp"

float ManagedEngine::GetGamma()
{
	return Engine::GetGamma();
}

void ManagedEngine::SetGamma(float gamma)
{
	Engine::SetGamma(gamma);
}