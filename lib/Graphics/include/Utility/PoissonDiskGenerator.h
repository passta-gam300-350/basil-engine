/******************************************************************************/
/*!
\file   PoissonDiskGenerator.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Poisson disk sample generation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <vector>

namespace PoissonDiskGenerator {
    /**
     * Generate Poisson disk distributed offsets for shadow sampling
     *
     * @param windowSize Size of the tiling pattern (typically 4-16)
     * @param filterSize Number of samples per dimension (4 = 16 samples total)
     * @return Flat array of float offsets (x,y pairs)
     */
    std::vector<float> GenerateOffsetTextureData(int windowSize, int filterSize);
}
