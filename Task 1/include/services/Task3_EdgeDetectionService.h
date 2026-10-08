#pragma once

#include <opencv2/core.hpp>

#include "services/TaskTypes.h"

// ----------------------------------------------------------------------------
// Task 3 - Edge Detection
//
// Constraints (graded!):
//   * Sobel, Roberts and Prewitt must be implemented MANUALLY - do NOT use
//     cv::Sobel / cv::filter2D high level wrappers for them; build the masks
//     yourself and convolve (cv::filter2D on your own kernel is acceptable).
//   * Every one of them must be able to produce the X and the Y direction
//     separately (the UI previews them one at a time via Direction).
//   * ONLY Canny may use OpenCV's built-in function: cv::Canny.
//     Canny does not need separate X/Y previews.
// ----------------------------------------------------------------------------
class Task3_EdgeDetectionService
{
public:
    Task3_EdgeDetectionService() = delete;

    // Sobel edges. direction = X  -> gradient in X (Gx)
    //                direction = Y  -> gradient in Y (Gy)
    static cv::Mat sobel(const cv::Mat& image, Direction direction);

    // Roberts cross gradient, X or Y direction.
    static cv::Mat roberts(const cv::Mat& image, Direction direction);

    // Prewitt operator, X or Y direction.
    static cv::Mat prewitt(const cv::Mat& image, Direction direction);

    // Canny - the ONLY OpenCV built-in function allowed in this task.
    // lowThreshold / highThreshold are in the 0..255 range.
    static cv::Mat canny(const cv::Mat& image, double lowThreshold, double highThreshold);
};
