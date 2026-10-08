#pragma once

#include <opencv2/core.hpp>

// ----------------------------------------------------------------------------
// Task 4 - Histograms
// Draw the histogram of an image together with its distribution curve
// (cumulative distribution function, CDF).
// Both functions return a ready-to-display image (dark background, axes,
// bars / polyline) so the controller can show them side by side.
// ----------------------------------------------------------------------------
class Task4_HistogramService
{
public:
    Task4_HistogramService() = delete;

    // Bar chart of the gray level distribution (0..255 bins).
    static cv::Mat drawHistogram(const cv::Mat& image);

    // Cumulative distribution function (the curve used for equalization).
    static cv::Mat drawCumulativeCurve(const cv::Mat& image);
};
