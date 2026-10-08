#include "services/Task8_ColorTransformationService.h"

// ============================================================================
// Task 8 - Color Processing & Channel Histograms               [TODO: Teammate]
// RGB -> grayscale, plus the individual R/G/B histograms and their
// cumulative curves (the mapping curves used for equalization).
// ============================================================================
cv::Mat Task8_ColorTransformationService::toGrayscale(const cv::Mat& image)
{
    // TODO(Task8): color -> grayscale (cv::cvtColor with COLOR_BGR2GRAY).
    return image;
}

cv::Mat Task8_ColorTransformationService::drawChannelHistograms(const cv::Mat& image)
{
    // TODO(Task8): split the channels (cv::split), compute one histogram per
    // channel and draw R, G and B side by side (use the channel colors).
    return image;
}

cv::Mat Task8_ColorTransformationService::drawChannelCumulativeCurves(const cv::Mat& image)
{
    // TODO(Task8): draw the cumulative distribution curve of each channel.
    return image;
}
