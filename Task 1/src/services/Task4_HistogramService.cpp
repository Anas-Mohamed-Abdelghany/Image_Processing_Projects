#include "services/Task4_HistogramService.h"

// ============================================================================
// Task 4 - Histograms                                          [TODO: Teammate]
// Draw the histogram AND the distribution curve (CDF) for an image.
// Return ready-to-display cv::Mat images (the controller shows them side by
// side in the UI).
// ============================================================================
cv::Mat Task4_HistogramService::drawHistogram(const cv::Mat& image)
{
    // TODO(Task4): compute the 256-bin histogram (cv::calcHist) and draw it
    // with axes and bar chart into a new cv::Mat.
    return image;
}

cv::Mat Task4_HistogramService::drawCumulativeCurve(const cv::Mat& image)
{
    // TODO(Task4): build the CDF from the histogram and draw it as a curve
    // (this is the mapping curve used for equalization in Task 5).
    return image;
}
