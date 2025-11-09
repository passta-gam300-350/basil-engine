/******************************************************************************/
/*!
\file   MaterialInstanceManager.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2025/10/20
\brief  Implementation of MaterialInstanceManager

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#include <Resources/MaterialInstanceManager.h>
#include <spdlog/spdlog.h>

std::shared_ptr<Material> MaterialInstanceManager::GetSharedMaterial(
    std::shared_ptr<Material> baseMaterial) const
{
    // Simply return the base material (no instancing)
    return baseMaterial;
}

std::shared_ptr<MaterialInstance> MaterialInstanceManager::GetMaterialInstance(
    ObjectID objectID,
    std::shared_ptr<Material> baseMaterial)
{
    if (!baseMaterial) {
        spdlog::error("MaterialInstanceManager: Cannot create instance from null base material");
        return nullptr;
    }

    // Check if instance already exists
    auto it = m_Instances.find(objectID);
    if (it != m_Instances.end()) {
        return it->second;
    }

    // Create new instance
    auto instance = std::make_shared<MaterialInstance>(baseMaterial);
    m_Instances[objectID] = instance;

    spdlog::debug("MaterialInstanceManager: Created instance for object {} from material '{}'",
                  objectID, baseMaterial->GetName());

    return instance;
}

bool MaterialInstanceManager::HasInstance(ObjectID objectID) const {
    return m_Instances.find(objectID) != m_Instances.end();
}

std::shared_ptr<MaterialInstance> MaterialInstanceManager::GetExistingInstance(ObjectID objectID) const {
    auto it = m_Instances.find(objectID);
    return (it != m_Instances.end()) ? it->second : nullptr;
}

void MaterialInstanceManager::SetSharedMaterial(ObjectID objectID, std::shared_ptr<Material> baseMaterial) {
    // Destroy existing instance if any
    DestroyInstance(objectID);

    spdlog::debug("MaterialInstanceManager: Set shared material '{}' for object {}",
                  baseMaterial ? baseMaterial->GetName() : "null",
                  objectID);
}

void MaterialInstanceManager::DestroyInstance(ObjectID objectID) {
    auto it = m_Instances.find(objectID);
    if (it != m_Instances.end()) {
        spdlog::debug("MaterialInstanceManager: Destroyed instance for object {}",
                      objectID);
        m_Instances.erase(it);
    }
}

void MaterialInstanceManager::ClearAllInstances() {
    size_t count = m_Instances.size();
    m_Instances.clear();

    if (count > 0) {
        spdlog::info("MaterialInstanceManager: Cleared {} material instances", count);
    }
}
