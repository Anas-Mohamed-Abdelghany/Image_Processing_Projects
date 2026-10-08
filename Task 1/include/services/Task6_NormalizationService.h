#pragma once

#include <opencv2/core.hpp>

// ----------------------------------------------------------------------------
// Task 6 - Image Normalization
// Normalize the pixel values of the image (e.g. to the full 0..255 range).
// ----------------------------------------------------------------------------
class Task6_NormalizationService
{
public:
    Task6_NormalizationService() = delete;

    // Returns the normalized image.
    static cv::Mat normalize(const cv::Mat& image);
};
