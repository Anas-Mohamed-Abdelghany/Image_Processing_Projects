#include "services/Task3_EdgeDetectionService.h"

#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>

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

// ---------------------------------------------------------------------------
// Manual helpers (plain loops, no OpenCV filtering functions)
// ---------------------------------------------------------------------------

// Single-channel 8-bit copy of the image (BGR/BGRA -> gray done by hand).
static cv::Mat toGrayManual(const cv::Mat& image)
{
    const int cn = image.channels();
    if (cn == 1)
        return image.clone();

    cv::Mat gray(image.rows, image.cols, CV_8UC1);
    for (int y = 0; y < image.rows; ++y)
    {
        const uchar* src = image.ptr<uchar>(y);
        uchar* dst = gray.ptr<uchar>(y);
        for (int x = 0; x < image.cols; ++x)
        {
            const uchar b = src[x * cn + 0];
            const uchar g = src[x * cn + 1];
            const uchar r = src[x * cn + 2];
            dst[x] = static_cast<uchar>(0.114 * b + 0.587 * g + 0.299 * r + 0.5);
        }
    }
    return gray;
}

// True 2D convolution of a gray image with a square ksize x ksize mask
// (row-major array), replicated borders, |result| clamped to 0..255.
static cv::Mat convolveAbs(const cv::Mat& gray, const int* mask, int ksize)
{
    const int anchor = ksize / 2;           // 3x3 -> centre, 2x2 -> (1,1)
    cv::Mat result(gray.rows, gray.cols, CV_8UC1);

    for (int y = 0; y < gray.rows; ++y)
    {
        uchar* dst = result.ptr<uchar>(y);
        for (int x = 0; x < gray.cols; ++x)
        {
            int sum = 0;
            for (int i = 0; i < ksize; ++i)
            {
                const int yy = std::clamp(y + anchor - i, 0, gray.rows - 1);
                const uchar* row = gray.ptr<uchar>(yy);
                for (int j = 0; j < ksize; ++j)
                {
                    const int xx = std::clamp(x + anchor - j, 0, gray.cols - 1);
                    sum += mask[i * ksize + j] * row[xx];
                }
            }
            dst[x] = static_cast<uchar>(std::min(std::abs(sum), 255));
        }
    }
    return result;
}

cv::Mat Task3_EdgeDetectionService::sobel(const cv::Mat& image, Direction direction)
{
    // Task3: manual Sobel masks for Gx (Direction::X) and Gy (Direction::Y),
    // convolve, take the absolute value and convert to 8 bit.
    if (image.empty())
        return image.clone();

    static const int Gx[9] = { -1, 0, 1,
                               -2, 0, 2,
                               -1, 0, 1 };
    static const int Gy[9] = { -1, -2, -1,
                                0,  0,  0,
                                1,  2,  1 };

    cv::Mat gray = toGrayManual(image);
    cv::Mat result = convolveAbs(gray, direction == Direction::X ? Gx : Gy, 3);
    return result;
}

cv::Mat Task3_EdgeDetectionService::roberts(const cv::Mat& image, Direction direction)
{
    // Task3: manual Roberts cross masks (2x2), X and Y directions.
    if (image.empty())
        return image.clone();

    static const int Gx[4] = { 1,  0,
                               0, -1 };
    static const int Gy[4] = { 0,  1,
                              -1,  0 };

    cv::Mat gray = toGrayManual(image);
    cv::Mat result = convolveAbs(gray, direction == Direction::X ? Gx : Gy, 2);
    return result;
}

cv::Mat Task3_EdgeDetectionService::prewitt(const cv::Mat& image, Direction direction)
{
    // Task3: manual Prewitt masks (3x3), X and Y directions.
    if (image.empty())
        return image.clone();

    static const int Gx[9] = { -1, 0, 1,
                               -1, 0, 1,
                               -1, 0, 1 };
    static const int Gy[9] = { -1, -1, -1,
                                0,  0,  0,
                                1,  1,  1 };

    cv::Mat gray = toGrayManual(image);
    cv::Mat result = convolveAbs(gray, direction == Direction::X ? Gx : Gy, 3);
    return result;
}

cv::Mat Task3_EdgeDetectionService::canny(const cv::Mat& image, double lowThreshold, double highThreshold)
{
    // Task3: cv::Canny(image, edges, lowThreshold, highThreshold)
    // (convert to grayscale first if needed).
    if (image.empty())
        return image.clone();

    cv::Mat gray;
    if (image.channels() == 1)
        gray = image;
    else if (image.channels() == 3)
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    else
        cv::cvtColor(image, gray, cv::COLOR_BGRA2GRAY);

    cv::Mat edges;
    cv::Canny(gray, edges, lowThreshold, highThreshold);
    return edges;
}
