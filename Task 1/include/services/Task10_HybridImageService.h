#pragma once

#include <opencv2/core.hpp>

// ----------------------------------------------------------------------------
// Task 10 - Hybrid Images
// A hybrid image combines
//   * the low-frequency components of one image   (Task 9 low pass filter)  with
//   * the high-frequency components of a second image (Task 9 high pass filter).
// Viewed up close it looks like the second image, from far away like the first.
// ----------------------------------------------------------------------------
class Task10_HybridImageService
{
public:
    Task10_HybridImageService() = delete;

    // lowImage  -> contributes its low frequencies
    // highImage -> contributes its high frequencies
    // cutoffRadius -> shared cut-off of the two frequency filters
    // Implement this by reusing Task9_FrequencyFilterService.
    static cv::Mat createHybrid(const cv::Mat& lowImage, const cv::Mat& highImage, double cutoffRadius);
};
