#include "services/Task1_NoiseService.h"

#include <algorithm>
#include <random>

// ============================================================================
// Task 1 - Additive Noise                                       [TODO: Teammate]
// Uniform noise / Gaussian noise / salt & pepper noise (OpenCV allowed).
// ============================================================================
cv::Mat Task1_NoiseService::addUniformNoise(const cv::Mat& image, int amplitude)
{
    // Task1: add uniform noise in [-amplitude, +amplitude]
    // (e.g. cv::randu on a noise image, then add and saturate).
    if (image.empty() || amplitude <= 0)
        return image.clone();

    const int cn = image.channels();

    // Signed 16-bit so negative noise / overflow doesn't wrap around.
    cv::Mat image16;
    image.convertTo(image16, CV_MAKETYPE(CV_16S, cn));

    cv::Mat noise(image.size(), image16.type());
    // For integer types randu draws from [low, high), hence amplitude + 1.
    cv::randu(noise, cv::Scalar::all(-amplitude), cv::Scalar::all(amplitude + 1));

    cv::Mat sum, result;
    cv::add(image16, noise, sum);
    sum.convertTo(result, image.depth());   // saturates to 0..255
    return result;
}

cv::Mat Task1_NoiseService::addGaussianNoise(const cv::Mat& image, double mean, double stddev)
{
    // Task1: add Gaussian noise, e.g. with cv::randn / cv::randn-like noise
    // of the same size and type as the image, then saturate_add.
    if (image.empty() || stddev < 0.0)
        return image.clone();

    const int cn = image.channels();

    // Float so fractional noise values are kept before saturation.
    cv::Mat image32;
    image.convertTo(image32, CV_MAKETYPE(CV_32F, cn));

    cv::Mat noise(image.size(), image32.type());
    cv::randn(noise, cv::Scalar::all(mean), cv::Scalar::all(stddev));

    cv::Mat sum, result;
    cv::add(image32, noise, sum);
    sum.convertTo(result, image.depth());   // rounds + saturates to 0..255
    return result;
}

cv::Mat Task1_NoiseService::addSaltAndPepperNoise(const cv::Mat& image, double noiseProbability)
{
    // Task1: set noiseProbability/2 of the pixels to 0 and noiseProbability/2 to 255.
    // (built from scratch, no OpenCV noise functions)
    if (image.empty())
        return image.clone();

    const double p = std::clamp(noiseProbability, 0.0, 1.0);
    cv::Mat result = image.clone();
    if (p == 0.0)
        return result;

    static std::mt19937 gen{ std::random_device{}() };
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    const int cn = result.channels();

    for (int y = 0; y < result.rows; ++y)
    {
        uchar* row = result.ptr<uchar>(y);
        for (int x = 0; x < result.cols; ++x)
        {
            const double r = dist(gen);
            if (r < p * 0.5)                // pepper -> 0
            {
                for (int c = 0; c < cn; ++c)
                    row[x * cn + c] = 0;
            }
            else if (r < p)                 // salt -> 255
            {
                for (int c = 0; c < cn; ++c)
                    row[x * cn + c] = 255;
            }
        }
    }
    return result;
}