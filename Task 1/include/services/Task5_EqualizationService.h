#pragma once

#include <opencv2/core.hpp>

// ----------------------------------------------------------------------------
// Task 5 - Histogram Equalization
// Improve the contrast of the image by equalizing its histogram.
// Use the CDF built in Task 4 (or cv::equalizeHist).
// ----------------------------------------------------------------------------
class Task5_EqualizationService
{
public:
    Task5_EqualizationService() = delete;

    // Returns the contrast-enhanced image.
    static cv::Mat equalize(const cv::Mat& image);
};
