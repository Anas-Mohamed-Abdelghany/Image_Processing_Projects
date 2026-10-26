#pragma once
#include "Types.h"
#include <vector>

class CannyEdgeDetector {
public:
    struct Params {
        int kernelSize = 5;          // Must be an odd number (e.g., 3, 5, 7)
        double sigma = 1.4;          // Standard deviation for Gaussian kernel
        double lowThreshold = 20.0;  // Lower hysteresis threshold (0-255)
        double highThreshold = 50.0; // Higher hysteresis threshold (0-255)
    };

    // Complete Canny pipeline
    static GrayImage detectEdges(const GrayImage& input, const Params& params);

    // Core algorithmic steps (accessible individually if needed)
    static GrayImage applyGaussianBlur(const GrayImage& img, int ksize, double sigma);
    static void computeSobelGradients(const GrayImage& smoothed, GrayImage& magnitude, GrayImage& angle);
    static GrayImage nonMaximumSuppression(const GrayImage& magnitude, const GrayImage& angle);
    static GrayImage hysteresisThresholding(const GrayImage& nms, double lowThresh, double highThresh);

private:
    // Helper convolution method with boundary clamping
    static GrayImage convolve2D(const GrayImage& img, const std::vector<std::vector<double>>& kernel);
    static std::vector<std::vector<double>> createGaussianKernel(int size, double sigma);
};