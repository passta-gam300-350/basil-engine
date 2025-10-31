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
