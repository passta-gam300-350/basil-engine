#ifndef MANAGEDLIGHT_HPP
#define	MANAGEDLIGHT_HPP
#include <cstdint>
#include "Light.h"
#include "Render.h"

class ManagedLight
{
public:
	// Type
	static void SetLightType(uint64_t handle, int type);
	static int  GetLightType(uint64_t handle);

	// Enabled
	static bool GetEnabled(uint64_t handle);
	static void SetEnabled(uint64_t handle, bool v);

	// Color
	static float GetColorR(uint64_t handle);
	static float GetColorG(uint64_t handle);
	static float GetColorB(uint64_t handle);
	static void SetColor(uint64_t handle, float r, float g, float b);
	static void SetColorR(uint64_t handle, float r);
	static void SetColorG(uint64_t handle, float g);
	static void SetColorB(uint64_t handle, float b);

	// Intensity
	static float GetIntensity(uint64_t handle);
	static void SetIntensity(uint64_t handle, float v);

	// Direction
	static float GetDirectionX(uint64_t handle);
	static float GetDirectionY(uint64_t handle);
	static float GetDirectionZ(uint64_t handle);
	static void SetDirection(uint64_t handle, float x, float y, float z);

	// Range
	static float GetRange(uint64_t handle);
	static void SetRange(uint64_t handle, float v);

	// Spot cones (degrees)
	static float GetInnerConeDeg(uint64_t handle);
	static float GetOuterConeDeg(uint64_t handle);
	static void SetCones(uint64_t handle, float innerDeg, float outerDeg);
};

#endif // MANAGEDLIGHT_HPP