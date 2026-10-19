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

    // Hybrid with ONE FILTER AND CUTOFF PER IMAGE (used by the hybrid page):
    // each image is low / high pass filtered with its own cutoff (Task 9), then
    // the two filtered images are combined:
    //   low pass  + high pass -> sum of the two          (the classic hybrid)
    //   low pass  + low pass  -> average of the two
    //   high pass + high pass -> the two details added on a mid gray background
    // image2 is fitted to the size of image1. Result: 8 bit, gray or BGR.
    static cv::Mat createHybridFromFilters(const cv::Mat& image1, bool highPass1, double cutoff1,
                                           const cv::Mat& image2, bool highPass2, double cutoff2);
};
