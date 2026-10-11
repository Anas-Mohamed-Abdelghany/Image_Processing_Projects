#include "services/Task2_LowPassFilterService.h"

#include <opencv2/imgproc.hpp>
#include <algorithm>

// ============================================================================
// Task 2 - Low Pass Filtering (Noise Reduction)                [TODO: Teammate]
// Average filter / Gaussian filter / Median filter - test with 3x3 and 5x5.
// ============================================================================

// Kernel sizes must be odd (3, 5, 7, ...). Even values are bumped up by one.
static int makeOdd(int ksize, int minSize)
{
    ksize = std::max(ksize, minSize);
    return (ksize % 2 == 0) ? ksize + 1 : ksize;
}

cv::Mat Task2_LowPassFilterService::applyAverageFilter(const cv::Mat& image, int ksize)
{
    // Task2: average filter (cv::blur or a manually built box kernel).
    if (image.empty())
        return image.clone();

    ksize = makeOdd(ksize, 1);

    cv::Mat result;
    cv::blur(image, result, cv::Size(ksize, ksize));
    return result;
}

cv::Mat Task2_LowPassFilterService::applyGaussianFilter(const cv::Mat& image, int ksize, double sigma)
{
    // Task2: Gaussian filter (cv::GaussianBlur).
    if (image.empty())
        return image.clone();

    ksize = makeOdd(ksize, 1);

    // If sigma <= 0, OpenCV derives it from the kernel size.
    cv::Mat result;
    cv::GaussianBlur(image, result, cv::Size(ksize, ksize), sigma);
    return result;
}

cv::Mat Task2_LowPassFilterService::applyMedianFilter(const cv::Mat& image, int ksize)
{
    // Task2: median filter (cv::medianBlur) - best against salt & pepper.
    if (image.empty())
        return image.clone();

    ksize = makeOdd(ksize, 3);              // medianBlur needs odd ksize > 1

    cv::Mat result;
    cv::medianBlur(image, result, ksize);
    return result;
}