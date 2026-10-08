#pragma once

#include <opencv2/core.hpp>

// ----------------------------------------------------------------------------
// Task 7 - Thresholding
// Apply any thresholding method to the image (global / simple threshold is
// fine - the specific local/global requirement was crossed out in the task
// sheet, so a plain threshold method is accepted).
// ----------------------------------------------------------------------------
class Task7_ThresholdingService
{
public:
    Task7_ThresholdingService() = delete;

    // thresholdValue and maxValue are in the 0..255 range (validated upstream).
    // The implementation decides which cv::ThresholdTypes value it uses.
    static cv::Mat applyThreshold(const cv::Mat& image, double thresholdValue, double maxValue);
};
