#pragma once

#include "../../Resources/Mesh.h"
#include <memory>

struct MeshComponent {
    std::shared_ptr<Mesh> Mesh;

    MeshComponent() = default;
    MeshComponent(const std::shared_ptr<Mesh>& mesh)
        : Mesh(mesh) {
    }
};