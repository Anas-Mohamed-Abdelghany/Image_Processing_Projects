#include "services/Task1_NoiseService.h"

// ============================================================================
// Task 1 - Additive Noise                                       [TODO: Teammate]
// Uniform noise / Gaussian noise / salt & pepper noise (OpenCV allowed).
// ============================================================================
cv::Mat Task1_NoiseService::addUniformNoise(const cv::Mat& image, int amplitude)
{
    // TODO(Task1): add uniform noise in [-amplitude, +amplitude]
    // (e.g. cv::randu on a noise image, then add and saturate).
    (void)amplitude;
    return image;
}

cv::Mat Task1_NoiseService::addGaussianNoise(const cv::Mat& image, double mean, double stddev)
{
    // TODO(Task1): add Gaussian noise, e.g. with cv::randn / cv::randn-like noise
    // of the same size and type as the image, then saturate_add.
    (void)mean;
    (void)stddev;
    return image;
}

cv::Mat Task1_NoiseService::addSaltAndPepperNoise(const cv::Mat& image, double noiseProbability)
{
    // TODO(Task1): set noiseProbability/2 of the pixels to 0 and noiseProbability/2 to 255.
    (void)noiseProbability;
    return image;
}
