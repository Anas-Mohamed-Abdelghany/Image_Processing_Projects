#include "services/Task3_EdgeDetectionService.h"

// ============================================================================
// Task 3 - Edge Detection                                       [TODO: Teammate]
//
// CRITICAL RULES:
//   * Sobel / Roberts / Prewitt: implement the masks MANUALLY and produce the
//     X and Y directions SEPARATELY (Direction::X / Direction::Y).
//     Do NOT call cv::Sobel() - build the kernel yourself and convolve.
//   * Canny: the ONLY place where an OpenCV built-in function is allowed
//     (cv::Canny).
// ============================================================================
cv::Mat Task3_EdgeDetectionService::sobel(const cv::Mat& image, Direction direction)
{
    // TODO(Task3): manual Sobel masks for Gx (Direction::X) and Gy (Direction::Y),
    // convolve, take the absolute value and convert to 8 bit.
    (void)direction;
    return image;
}

cv::Mat Task3_EdgeDetectionService::roberts(const cv::Mat& image, Direction direction)
{
    // TODO(Task3): manual Roberts cross masks (2x2), X and Y directions.
    (void)direction;
    return image;
}

cv::Mat Task3_EdgeDetectionService::prewitt(const cv::Mat& image, Direction direction)
{
    // TODO(Task3): manual Prewitt masks (3x3), X and Y directions.
    (void)direction;
    return image;
}

cv::Mat Task3_EdgeDetectionService::canny(const cv::Mat& image, double lowThreshold, double highThreshold)
{
    // TODO(Task3): cv::Canny(image, edges, lowThreshold, highThreshold)
    // (convert to grayscale first if needed).
    (void)lowThreshold;
    (void)highThreshold;
    return image;
}
