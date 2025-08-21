#pragma once

#include "../../Utility/Camera.h"
#include <memory>

struct CameraComponent
{
    std::shared_ptr<Camera> camera;
    bool Primary = true;
    bool FixedAspectRatio = false;

    CameraComponent() = default;
    CameraComponent(const std::shared_ptr<Camera>& camera)
        : camera(camera)
    {
    }
};