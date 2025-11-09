/******************************************************************************/
/*!
\file   ManagedParticle.hpp
\author Team PASSTA
		Hai Jie (haijie.w\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the declaration for the ManagedParticle class, which
provides an interface for managing particle properties in a managed (C#) environment.


Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MANAGEDPARTICLE_HPP
#define	MANAGEDPARTICLE_HPP
#include <cstdint>


class ManagedParticle
{
public:
	// ---- Playback (runtime) ----
	static void Play(uint64_t h);
	static void Stop(uint64_t h);
	static void Reset(uint64_t h);
	static bool IsPlaying(uint64_t h);
	static bool IsAlive(uint64_t h);
	static int GetActiveCount(uint64_t h);

	// ---- Component level flags ----
	static bool GetAutoPlay(uint64_t h);
	static void SetAutoPlay(uint64_t h, bool v);
	static int GetBlendMode(uint64_t h);
	static void SetBlendMode(uint64_t h, int v);
	static bool GetDepthWrite(uint64_t h);
	static void SetDepthWrite(uint64_t h, bool v);
	static uint32_t GetRenderLayer(uint64_t h);
	static void SetRenderLayer(uint64_t h, uint32_t v);

	// ---- Emitter configuration ----
	// Emission shape
	static int GetEmissionType(uint64_t h);
	static void SetEmissionType(uint64_t h, int v);
	static float GetEmitterSizeX(uint64_t h);
	static float GetEmitterSizeY(uint64_t h);
	static float GetEmitterSizeZ(uint64_t h);
	static void SetEmitterSize(uint64_t h, float x, float y, float z);
	static float GetSphereRadius(uint64_t h);
	static void SetSphereRadius(uint64_t h, float r);

	// Emission settings
	static float GetEmissionRate(uint64_t h);
	static void SetEmissionRate(uint64_t h, float v);
	static int GetMaxParticles(uint64_t h);
	static void SetMaxParticles(uint64_t h, int v);
	static bool GetLooping(uint64_t h);
	static void SetLooping(uint64_t h, bool v);
	static float GetDuration(uint64_t h);
	static void SetDuration(uint64_t h, float v);

	// Lifetime
	static float GetMinLifetime(uint64_t h);
	static void SetMinLifetime(uint64_t h, float v);
	static float GetMaxLifetime(uint64_t h);
	static void SetMaxLifetime(uint64_t h, float v);

	// Visuals
	static float GetStartColorR(uint64_t h);
	static float GetStartColorG(uint64_t h);
	static float GetStartColorB(uint64_t h);
	static float GetStartColorA(uint64_t h);
	static void SetStartColor(uint64_t h, float r, float g, float b, float a);
	static float GetEndColorR(uint64_t h);
	static float GetEndColorG(uint64_t h);
	static float GetEndColorB(uint64_t h);
	static float GetEndColorA(uint64_t h);
	static void SetEndColor(uint64_t h, float r, float g, float b, float a);
	static float GetStartSize(uint64_t h);
	static void SetStartSize(uint64_t h, float v);
	static float GetEndSize(uint64_t h);
	static void SetEndSize(uint64_t h, float v);
	static float GetStartRotation(uint64_t h);
	static void SetStartRotation(uint64_t h, float v);
	static float GetRotationSpeed(uint64_t h);
	static void SetRotationSpeed(uint64_t h, float v);

	// Physics
	static float GetStartVelX(uint64_t h);
	static float GetStartVelY(uint64_t h);
	static float GetStartVelZ(uint64_t h);
	static void SetStartVelocity(uint64_t h, float x, float y, float z);
	static float GetVelocityRandomness(uint64_t h);
	static void SetVelocityRandomness(uint64_t h, float v);
	static float GetAccelX(uint64_t h);
	static float GetAccelY(uint64_t h);
	static float GetAccelZ(uint64_t h);
	static void SetAcceleration(uint64_t h, float x, float y, float z);
};

#endif // MANAGEDPARTICLE_HPP