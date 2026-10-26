#include "CannyEdgeDetector.h"
#include <cmath>
#include <queue>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// MAIN PIPELINE
// ============================================================================

GrayImage CannyEdgeDetector::detectEdges(const GrayImage& input, const Params& params) {
    if (input.empty() || input[0].empty()) return {};

    // 1. Noise Reduction via Gaussian Filter
    GrayImage smoothed = applyGaussianBlur(input, params.kernelSize, params.sigma);

    // 2. Compute Gradient Magnitude and Angle via Sobel Operators
    GrayImage magnitude, angle;
    computeSobelGradients(smoothed, magnitude, angle);

    // 3. Thin edges using Non-Maximum Suppression (NMS)
    GrayImage nms = nonMaximumSuppression(magnitude, angle);

    // 4. Double Thresholding and Edge Tracking via Hysteresis
    return hysteresisThresholding(nms, params.lowThreshold, params.highThreshold);
}

// ============================================================================
// STEP 1: GAUSSIAN BLUR
// ============================================================================

std::vector<std::vector<double>> CannyEdgeDetector::createGaussianKernel(int size, double sigma) {
    if (size % 2 == 0) size += 1; // Ensure odd size
    int radius = size / 2;
    std::vector<std::vector<double>> kernel(size, std::vector<double>(size, 0.0));
    double sum = 0.0;
    double twoSigmaSq = 2.0 * sigma * sigma;

    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            double val = std::exp(-(x * x + y * y) / twoSigmaSq) / (M_PI * twoSigmaSq);
            kernel[y + radius][x + radius] = val;
            sum += val;
        }
    }

    // Normalize kernel so weights sum to 1.0
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            kernel[y][x] /= sum;
        }
    }
    return kernel;
}

GrayImage CannyEdgeDetector::convolve2D(const GrayImage& img, const std::vector<std::vector<double>>& kernel) {
    int H = static_cast<int>(img.size());
    int W = static_cast<int>(img[0].size());
    int kH = static_cast<int>(kernel.size());
    int kW = static_cast<int>(kernel[0].size());
    int padY = kH / 2;
    int padX = kW / 2;

    GrayImage output(H, std::vector<double>(W, 0.0));

    for (int r = 0; r < H; ++r) {
        for (int c = 0; c < W; ++c) {
            double sum = 0.0;
            for (int kr = 0; kr < kH; ++kr) {
                for (int kc = 0; kc < kW; ++kc) {
                    // Replicate boundary clamp for image edges
                    int imgR = std::clamp(r + kr - padY, 0, H - 1);
                    int imgC = std::clamp(c + kc - padX, 0, W - 1);
                    sum += img[imgR][imgC] * kernel[kr][kc];
                }
            }
            output[r][c] = sum;
        }
    }
    return output;
}

GrayImage CannyEdgeDetector::applyGaussianBlur(const GrayImage& img, int ksize, double sigma) {
    auto kernel = createGaussianKernel(ksize, sigma);
    return convolve2D(img, kernel);
}

// ============================================================================
// STEP 2: SOBEL GRADIENTS
// ============================================================================

void CannyEdgeDetector::computeSobelGradients(
    const GrayImage& smoothed,
    GrayImage& magnitude,
    GrayImage& angle) 
{
    int H = static_cast<int>(smoothed.size());
    int W = static_cast<int>(smoothed[0].size());

    // Standard Sobel kernels
    std::vector<std::vector<double>> Kx = {
        {-1.0, 0.0, 1.0},
        {-2.0, 0.0, 2.0},
        {-1.0, 0.0, 1.0}
    };

    std::vector<std::vector<double>> Ky = {
        {-1.0, -2.0, -1.0},
        { 0.0,  0.0,  0.0},
        { 1.0,  2.0,  1.0}
    };

    GrayImage Gx = convolve2D(smoothed, Kx);
    GrayImage Gy = convolve2D(smoothed, Ky);

    magnitude = GrayImage(H, std::vector<double>(W, 0.0));
    angle = GrayImage(H, std::vector<double>(W, 0.0));

    for (int r = 0; r < H; ++r) {
        for (int c = 0; c < W; ++c) {
            magnitude[r][c] = std::hypot(Gx[r][c], Gy[r][c]);

            // Direction in degrees [0, 180)
            double theta = std::atan2(Gy[r][c], Gx[r][c]) * 180.0 / M_PI;
            if (theta < 0.0) theta += 180.0;
            angle[r][c] = theta;
        }
    }
}

// ============================================================================
// STEP 3: NON-MAXIMUM SUPPRESSION (NMS)
// ============================================================================

GrayImage CannyEdgeDetector::nonMaximumSuppression(const GrayImage& magnitude, const GrayImage& angle) {
    int H = static_cast<int>(magnitude.size());
    int W = static_cast<int>(magnitude[0].size());
    GrayImage nms(H, std::vector<double>(W, 0.0));

    for (int r = 1; r < H - 1; ++r) {
        for (int c = 1; c < W - 1; ++c) {
            double current = magnitude[r][c];
            if (current <= 1e-4) continue;

            double theta = angle[r][c];
            double neighbor1 = 0.0;
            double neighbor2 = 0.0;

            // Sector 0: 0 degrees (East-West / Horizontal normal)
            if ((theta >= 0.0 && theta < 22.5) || (theta >= 157.5 && theta <= 180.0)) {
                neighbor1 = magnitude[r][c + 1];
                neighbor2 = magnitude[r][c - 1];
            }
            // Sector 1: 45 degrees (Diagonal / Down-Right & Up-Left)
            else if (theta >= 22.5 && theta < 67.5) {
                neighbor1 = magnitude[r + 1][c + 1];
                neighbor2 = magnitude[r - 1][c - 1];
            }
            // Sector 2: 90 degrees (North-South / Vertical normal)
            else if (theta >= 67.5 && theta < 112.5) {
                neighbor1 = magnitude[r + 1][c];
                neighbor2 = magnitude[r - 1][c];
            }
            // Sector 3: 135 degrees (Diagonal / Down-Left & Up-Right)
            else if (theta >= 112.5 && theta < 157.5) {
                neighbor1 = magnitude[r + 1][c - 1];
                neighbor2 = magnitude[r - 1][c + 1];
            }

            // Keep only if local maximum along the gradient normal
            if (current >= neighbor1 && current >= neighbor2) {
                nms[r][c] = current;
            } else {
                nms[r][c] = 0.0;
            }
        }
    }
    return nms;
}

// ============================================================================
// STEP 4: DOUBLE THRESHOLDING & HYSTERESIS
// ============================================================================

GrayImage CannyEdgeDetector::hysteresisThresholding(
    const GrayImage& nms, 
    double lowThresh, 
    double highThresh) 
{
    int H = static_cast<int>(nms.size());
    int W = static_cast<int>(nms[0].size());

    GrayImage result(H, std::vector<double>(W, 0.0));
    std::queue<std::pair<int, int>> strongQueue;

    // 1. Identify Strong and Weak edges
    // 0: Non-edge, 100: Weak candidate, 255: Strong edge
    for (int r = 0; r < H; ++r) {
        for (int c = 0; c < W; ++c) {
            double val = nms[r][c];
            if (val >= highThresh) {
                result[r][c] = 255.0;
                strongQueue.push({r, c});
            } else if (val >= lowThresh) {
                result[r][c] = 100.0; // Mark candidate
            } else {
                result[r][c] = 0.0;
            }
        }
    }

    // 2. BFS Edge-Tracking: Weak edges connected to strong edges become strong
    const int dr[8] = {-1, -1, -1,  0, 0,  1, 1, 1};
    const int dc[8] = {-1,  0,  1, -1, 1, -1, 0, 1};

    while (!strongQueue.empty()) {
        auto [cr, cc] = strongQueue.front();
        strongQueue.pop();

        for (int i = 0; i < 8; ++i) {
            int nr = cr + dr[i];
            int nc = cc + dc[i];

            if (nr >= 0 && nr < H && nc >= 0 && nc < W) {
                if (result[nr][nc] == 100.0) { // Connect weak edge
                    result[nr][nc] = 255.0;
                    strongQueue.push({nr, nc});
                }
            }
        }
    }

    // 3. Suppress all remaining disconnected weak edges
    for (int r = 0; r < H; ++r) {
        for (int c = 0; c < W; ++c) {
            if (result[r][c] != 255.0) {
                result[r][c] = 0.0;
            }
        }
    }

    return result;
}