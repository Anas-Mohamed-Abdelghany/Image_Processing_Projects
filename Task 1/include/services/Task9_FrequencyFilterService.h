#pragma once

#include <opencv2/core.hpp>

// ----------------------------------------------------------------------------
// Task 9 - Frequency Domain Filtering
// Implement high pass and low pass filters in the frequency domain
// (FFT -> build the filter mask -> inverse FFT).
// cutoffRadius is the radius of the low / high pass region in the centered
// spectrum, chosen in the UI.
// ----------------------------------------------------------------------------
class Task9_FrequencyFilterService
{
public:
    Task9_FrequencyFilterService() = delete;

    // Keeps only the low frequencies (smoothed image).
    static cv::Mat lowPass(const cv::Mat& image, double cutoffRadius);

    // Keeps only the high frequencies (edges / detail).
    static cv::Mat highPass(const cv::Mat& image, double cutoffRadius);
};
