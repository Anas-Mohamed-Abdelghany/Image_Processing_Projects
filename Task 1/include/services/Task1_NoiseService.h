#pragma once

#include <opencv2/core.hpp>

// ----------------------------------------------------------------------------
// Task 1 - Additive Noise
// Add different noise types to the input image (OpenCV functions allowed):
//   * uniform noise
//   * Gaussian noise
//   * salt & pepper noise
// Parameters are chosen in the UI, so keep them as arguments.
// ----------------------------------------------------------------------------
class Task1_NoiseService
{
public:
    Task1_NoiseService() = delete;

    // Uniform noise: perturb every pixel by a random value in [-amplitude, +amplitude].
    // amplitude range: 1..255 (validated by InputValidator).
    static cv::Mat addUniformNoise(const cv::Mat& image, int amplitude);

    // Gaussian noise with the given mean and standard deviation (in gray levels).
    static cv::Mat addGaussianNoise(const cv::Mat& image, double mean, double stddev);

    // Salt & pepper noise: noiseProbability (0..1) is the fraction of pixels
    // that are replaced with pure white (salt) or black (pepper).
    static cv::Mat addSaltAndPepperNoise(const cv::Mat& image, double noiseProbability);
};
