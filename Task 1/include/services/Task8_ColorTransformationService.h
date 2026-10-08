#pragma once

#include <opencv2/core.hpp>

// ----------------------------------------------------------------------------
// Task 8 - Color Processing & Channel Histograms
//   * transform a color (RGB) image into a grayscale image
//   * plot the individual histograms of the R, G and B channels
//   * plot the distribution function (cumulative curve) of each channel -
//     this is the curve used for mapping and equalization
// ----------------------------------------------------------------------------
class Task8_ColorTransformationService
{
public:
    Task8_ColorTransformationService() = delete;

    // RGB -> grayscale (the input is a BGR cv::Mat, see QtCvBridge).
    static cv::Mat toGrayscale(const cv::Mat& image);

    // Ready-to-display image with the R, G and B histograms side by side.
    static cv::Mat drawChannelHistograms(const cv::Mat& image);

    // Ready-to-display image with the R, G and B cumulative curves side by side.
    static cv::Mat drawChannelCumulativeCurves(const cv::Mat& image);
};
