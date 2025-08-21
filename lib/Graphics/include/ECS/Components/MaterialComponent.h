#pragma once

#include "../../Resources/Material.h"
#include <memory>
#include <vector>

struct MaterialComponent {
    std::vector<std::shared_ptr<Material>> Materials;

    MaterialComponent() = default;
    MaterialComponent(const std::shared_ptr<Material>& material) {
        Materials.push_back(material);
    }
};