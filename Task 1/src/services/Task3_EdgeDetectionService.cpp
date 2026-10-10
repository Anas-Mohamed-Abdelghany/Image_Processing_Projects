#include "services/Task3_EdgeDetectionService.h"

#include <opencv2/imgproc.hpp>

#include <cmath>
#include <initializer_list>

// ============================================================================
// Task 3 - Edge Detection
//
// CRITICAL RULES:
//   * Sobel / Roberts / Prewitt: the masks AND the convolution are implemented
//     from scratch - no cv::Sobel, no cv::filter2D, no OpenCV helper at all.
//     Do NOT call cv::Sobel() - build the kernel yourself and convolve.
//   * Canny: the ONLY place where an OpenCV built-in function is allowed
//     (cv::Canny).
//   * Only cv::cvtColor (BGR -> gray, a colour conversion, not an edge
//     operator) is shared with OpenCV.
// ============================================================================

namespace {

// ---------------------------------------------------------------------------
// Every operator in this task works on a single channel, so the input is
// reduced to 8 bit grayscale first. That also guarantees the X and the Y
// preview are computed from exactly the same source image.
// ---------------------------------------------------------------------------
cv::Mat toGray8(const cv::Mat& image)
{
    if (image.empty()) {
        return {};
    }

    cv::Mat gray;
    if (image.channels() == 1) {
        gray = image;
    } else if (image.channels() == 4) {
        cv::cvtColor(image, gray, cv::COLOR_BGRA2GRAY);
    } else {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    }

    if (gray.depth() == CV_8U) {
        return gray;
    }

    cv::Mat gray8;
    if (gray.depth() == CV_16U) {
        gray.convertTo(gray8, CV_8U, 1.0 / 256.0);
    } else {
        // float / double input: stretch [min, max] to [0, 255].
        double minValue = 0.0;
        double maxValue = 0.0;
        cv::minMaxLoc(gray.reshape(1), &minValue, &maxValue);
        const double scale = (maxValue > minValue) ? 255.0 / (maxValue - minValue) : 1.0;
        gray.convertTo(gray8, CV_8U, scale, -minValue * scale);
    }
    return gray8;
}

// ---------------------------------------------------------------------------
// Builds a convolution mask from coefficients written out row by row. The
// masks are declared by hand on purpose - this task does not allow the
// OpenCV edge operators.
// ---------------------------------------------------------------------------
cv::Mat makeKernel(int rows, int cols, std::initializer_list<float> coefficients)
{
    cv::Mat kernel(rows, cols, CV_32F);
    auto coefficient = coefficients.begin();
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            kernel.at<float>(row, col) = *coefficient++;
        }
    }
    return kernel;
}

// ---------------------------------------------------------------------------
// Hand written convolution - the NEIGHBOURHOOD SUMMATION IS DONE HERE, NOT BY
// OPENCV. For every output pixel the mask is slid over the image, each mask
// coefficient is multiplied with the gray value below it and the products are
// accumulated. Out of image coordinates replicate the edge value (border
// handling is written out as well).
//
// Finally the absolute value of the response is clipped into 0..255, so the
// negative half of the gradient is folded around 0 and bright-on-dark as well
// as dark-on-bright edges show up. The accumulator is a float on purpose: an
// 8 bit accumulator would clip the negative mask coefficients before the
// absolute value could be taken.
//
// cv::filter2D / cv::absdiff / cv::convertTo are deliberately NOT used here -
// only cv::Canny may be a built-in in this task.
// ---------------------------------------------------------------------------
cv::Mat convolve(const cv::Mat& gray8, const cv::Mat& kernel)
{
    const int rows       = gray8.rows;
    const int cols       = gray8.cols;
    const int kernelRows = kernel.rows;
    const int kernelCols = kernel.cols;
    const int anchorY    = kernelRows / 2; // kernel centre
    const int anchorX    = kernelCols / 2;

    cv::Mat edges(rows, cols, CV_8UC1, cv::Scalar(0));

    for (int y = 0; y < rows; ++y) {
        uchar* outputRow = edges.ptr<uchar>(y);

        for (int x = 0; x < cols; ++x) {
            float response = 0.0f;

            for (int ky = 0; ky < kernelRows; ++ky) {
                int sourceY = y + ky - anchorY;
                if (sourceY < 0) {
                    sourceY = 0; // replicate top border
                } else if (sourceY >= rows) {
                    sourceY = rows - 1; // replicate bottom border
                }

                const uchar*  sourceRow  = gray8.ptr<uchar>(sourceY);
                const float*  kernelRow  = kernel.ptr<float>(ky);

                for (int kx = 0; kx < kernelCols; ++kx) {
                    int sourceX = x + kx - anchorX;
                    if (sourceX < 0) {
                        sourceX = 0; // replicate left border
                    } else if (sourceX >= cols) {
                        sourceX = cols - 1; // replicate right border
                    }

                    response += kernelRow[kx] * static_cast<float>(sourceRow[sourceX]);
                }
            }

            // |gradient| clipped into 0..255 (this replaces absdiff + convertTo)
            const float magnitude = std::fabs(response);
            outputRow[x] = (magnitude >= 255.0f) ? static_cast<uchar>(255)
                                                  : static_cast<uchar>(magnitude);
        }
    }

    return edges;
}

} // namespace

// ---------------------------------------------------------------------------
// Sobel 3x3
//   Gx (Direction::X) - derivative along X -> highlights VERTICAL edges
//   Gy (Direction::Y) - derivative along Y -> highlights HORIZONTAL edges
// ---------------------------------------------------------------------------
cv::Mat Task3_EdgeDetectionService::sobel(const cv::Mat& image, Direction direction)
{
    const cv::Mat gray = toGray8(image);
    if (gray.empty()) {
        return {};
    }

    cv::Mat kernel;
    if (direction == Direction::X) {
        // dI/dx -> vertical edges
        kernel = makeKernel(3, 3, {
            -1.0f, 0.0f, 1.0f,
            -2.0f, 0.0f, 2.0f,
            -1.0f, 0.0f, 1.0f});
    } else {
        // dI/dy -> horizontal edges
        kernel = makeKernel(3, 3, {
            -1.0f, -2.0f, -1.0f,
             0.0f,  0.0f,  0.0f,
             1.0f,  2.0f,  1.0f});
    }

    return convolve(gray, kernel);
}

// ---------------------------------------------------------------------------
// Roberts cross - 2x2 masks, the difference between diagonal neighbours:
//   Gx (Direction::X):  I(x, y)     - I(x+1, y+1)
//   Gy (Direction::Y):  I(x+1, y)   - I(x,   y+1)
// ---------------------------------------------------------------------------
cv::Mat Task3_EdgeDetectionService::roberts(const cv::Mat& image, Direction direction)
{
    const cv::Mat gray = toGray8(image);
    if (gray.empty()) {
        return {};
    }

    cv::Mat kernel;
    if (direction == Direction::X) {
        kernel = makeKernel(2, 2, {
             1.0f,  0.0f,
             0.0f, -1.0f});
    } else {
        kernel = makeKernel(2, 2, {
             0.0f,  1.0f,
            -1.0f,  0.0f});
    }

    return convolve(gray, kernel);
}

// ---------------------------------------------------------------------------
// Prewitt 3x3 - uniform average of the 3 neighbour pixels in the direction
// of the derivative (less noise sensitive than the 2x2 Roberts mask).
// ---------------------------------------------------------------------------
cv::Mat Task3_EdgeDetectionService::prewitt(const cv::Mat& image, Direction direction)
{
    const cv::Mat gray = toGray8(image);
    if (gray.empty()) {
        return {};
    }

    cv::Mat kernel;
    if (direction == Direction::X) {
        kernel = makeKernel(3, 3, {
            -1.0f, 0.0f, 1.0f,
            -1.0f, 0.0f, 1.0f,
            -1.0f, 0.0f, 1.0f});
    } else {
        kernel = makeKernel(3, 3, {
            -1.0f, -1.0f, -1.0f,
             0.0f,  0.0f,  0.0f,
             1.0f,  1.0f,  1.0f});
    }

    return convolve(gray, kernel);
}

// ---------------------------------------------------------------------------
// Canny - the ONLY OpenCV built-in function allowed in this task.
// Produces a binary image (0 / 255) with thin, single pixel wide edges, so it
// does not need separate X and Y previews.
// ---------------------------------------------------------------------------
cv::Mat Task3_EdgeDetectionService::canny(const cv::Mat& image, double lowThreshold, double highThreshold)
{
    const cv::Mat gray = toGray8(image);
    if (gray.empty()) {
        return {};
    }

    cv::Mat edges;
    cv::Canny(gray, edges, lowThreshold, highThreshold);
    return edges;
}
