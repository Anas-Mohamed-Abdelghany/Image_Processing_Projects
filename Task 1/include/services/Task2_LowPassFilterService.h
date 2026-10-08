#pragma once

#include <opencv2/core.hpp>

// ----------------------------------------------------------------------------
// Task 2 - Low Pass Filtering (Noise Reduction)
// Filter the noisy images produced by Task 1:
//   * average (mean) filter
//   * Gaussian filter
//   * median filter
// The kernel size is chosen in the UI (3x3, 5x5, 7x7, 9x9).
// ----------------------------------------------------------------------------
class Task2_LowPassFilterService
{
public:
    Task2_LowPassFilterService() = delete;

    // Average / mean filter with an odd ksize x ksize kernel (3, 5, 7 or 9).
    static cv::Mat applyAverageFilter(const cv::Mat& image, int ksize);

    // Gaussian filter: odd ksize x ksize kernel, standard deviation = sigma.
    static cv::Mat applyGaussianFilter(const cv::Mat& image, int ksize, double sigma);

    // Median filter: robust against salt & pepper noise, odd ksize x ksize kernel.
    static cv::Mat applyMedianFilter(const cv::Mat& image, int ksize);
};
