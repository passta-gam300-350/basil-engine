#include "Utility/PoissonDiskGenerator.h"
#include <random>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace PoissonDiskGenerator {

/**
 * Generate random jitter value for stratified sampling
 */
static float Jitter() {
    static std::default_random_engine generator;
    static std::uniform_real_distribution<float> distrib(-0.5f, 0.5f);
    return distrib(generator);
}

std::vector<float> GenerateOffsetTextureData(int windowSize, int filterSize) {
    // Buffer size: WindowSize × WindowSize patterns × FilterSize² samples × 2 (x,y)
    int bufferSize = windowSize * windowSize * filterSize * filterSize * 2;
    std::vector<float> data(bufferSize);

    int index = 0;

    // Generate offset patterns for screen-space tiling
    for (int texY = 0; texY < windowSize; texY++) {
        for (int texX = 0; texX < windowSize; texX++) {
            // Generate FilterSize × FilterSize samples per tile
            for (int v = filterSize - 1; v >= 0; v--) {
                for (int u = 0; u < filterSize; u++) {
                    // Stratified jittering: divide [0,1]×[0,1] into grid cells
                    float x = ((float)u + 0.5f + Jitter()) / (float)filterSize;
                    float y = ((float)v + 0.5f + Jitter()) / (float)filterSize;

                    // Convert to Poisson disk distribution using polar coordinates
                    // sqrt(y) ensures uniform distribution in circular area
                    float r = sqrtf(y);
                    float theta = 2.0f * (float)M_PI * x;

                    // Store Cartesian coordinates
                    data[index++] = r * cosf(theta);
                    data[index++] = r * sinf(theta);
                }
            }
        }
    }

    return data;
}

} // namespace PoissonDiskGenerator
