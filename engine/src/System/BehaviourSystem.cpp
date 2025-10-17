#include "MonoManager.hpp"
#include "System/BehaviourSystem.hpp"
#include "Manager/MonoEntityManager.hpp"

#include "ABI/ABI.h"

const char* defSearchDirs[] = {
	"Scripts/Bin/",
	"Scripts/Bin/Debug/",
	"Scripts/Bin/Release/"
};

const char* defAssembly = "Scripts/Bin/GameAssembly.dll";


void BehaviourSystem::Init()
{
	

}
void BehaviourSystem::Update(ecs::world& world, float dt)
{
}
void BehaviourSystem::FixedUpdate(ecs::world& world)
{
}
void BehaviourSystem::Exit()
{
}