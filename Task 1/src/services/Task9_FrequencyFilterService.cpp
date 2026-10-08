#include "services/Task9_FrequencyFilterService.h"

// ============================================================================
// Task 9 - Frequency Domain Filtering                          [TODO: Teammate]
// High pass filter and low pass filter implemented in the frequency domain:
//   FFT (cv::dft, shifted) -> build the mask around the center -> inverse FFT.
// ============================================================================
cv::Mat Task9_FrequencyFilterService::lowPass(const cv::Mat& image, double cutoffRadius)
{
    // TODO(Task9): keep everything inside a circle of cutoffRadius around the
    // spectrum center, then inverse FFT back to the spatial domain.
    (void)cutoffRadius;
    return image;
}

cv::Mat Task9_FrequencyFilterService::highPass(const cv::Mat& image, double cutoffRadius)
{
    // TODO(Task9): keep everything OUTSIDE a circle of cutoffRadius around the
    // spectrum center, then inverse FFT back to the spatial domain.
    (void)cutoffRadius;
    return image;
}
