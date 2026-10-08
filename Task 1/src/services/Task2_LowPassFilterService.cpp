#include "services/Task2_LowPassFilterService.h"

// ============================================================================
// Task 2 - Low Pass Filtering (Noise Reduction)                [TODO: Teammate]
// Average filter / Gaussian filter / Median filter - test with 3x3 and 5x5.
// ============================================================================
cv::Mat Task2_LowPassFilterService::applyAverageFilter(const cv::Mat& image, int ksize)
{
    // TODO(Task2): average filter (cv::blur or a manually built box kernel).
    (void)ksize;
    return image;
}

cv::Mat Task2_LowPassFilterService::applyGaussianFilter(const cv::Mat& image, int ksize, double sigma)
{
    // TODO(Task2): Gaussian filter (cv::GaussianBlur).
    (void)ksize;
    (void)sigma;
    return image;
}

cv::Mat Task2_LowPassFilterService::applyMedianFilter(const cv::Mat& image, int ksize)
{
    // TODO(Task2): median filter (cv::medianBlur) - best against salt & pepper.
    (void)ksize;
    return image;
}
