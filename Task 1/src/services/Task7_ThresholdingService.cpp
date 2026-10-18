#include "services/Task7_ThresholdingService.h"

#include <opencv2/imgproc.hpp>

// ============================================================================
// Task 7 - Thresholding
// Applies a global binary threshold (any thresholding method is accepted).
// ============================================================================
cv::Mat Task7_ThresholdingService::applyThreshold(const cv::Mat& image, double thresholdValue, double maxValue)
{
    if (image.empty())
    {
        return image;
    }

    // Thresholding operates on a single channel; convert color images to gray.
    cv::Mat gray;
    if (image.channels() == 3)
    {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    }
    else if (image.channels() == 4)
    {
        cv::cvtColor(image, gray, cv::COLOR_BGRA2GRAY);
    }
    else
    {
        gray = image;
    }

    // cv::threshold with THRESH_BINARY needs an 8-bit (or 32-bit float) input.
    if (gray.depth() != CV_8U)
    {
        cv::Mat converted;
        cv::normalize(gray, converted, 0, 255, cv::NORM_MINMAX, CV_8U);
        gray = converted;
    }

    // pixel > thresholdValue -> maxValue, otherwise 0
    cv::Mat result;
    cv::threshold(gray, result, thresholdValue, maxValue, cv::THRESH_BINARY);
    return result;
}