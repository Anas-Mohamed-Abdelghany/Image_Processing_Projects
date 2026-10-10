#include "services/Task5_EqualizationService.h"

#include <opencv2/imgproc.hpp>

#include <array>
#include <cmath>

// ============================================================================
// Task 5 - Histogram Equalization
// Improve contrast by equalizing the histogram (use your Task 4 CDF or
// cv::equalizeHist).
// ============================================================================

namespace {

using Histogram = std::array<int, 256>;

// ---------------------------------------------------------------------------
// Single channel statistics - the same reduction used by Task 4 so the CDF of
// the histogram preview and the mapping applied here always match.
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
        double minValue = 0.0;
        double maxValue = 0.0;
        cv::minMaxLoc(gray.reshape(1), &minValue, &maxValue);
        const double scale = (maxValue > minValue) ? 255.0 / (maxValue - minValue) : 1.0;
        gray.convertTo(gray8, CV_8U, scale, -minValue * scale);
    }
    return gray8;
}

Histogram computeHistogram(const cv::Mat& gray8)
{
    Histogram histogram{};
    histogram.fill(0);

    for (int row = 0; row < gray8.rows; ++row) {
        const uchar* pixels = gray8.ptr<uchar>(row);
        for (int col = 0; col < gray8.cols; ++col) {
            ++histogram[pixels[col]];
        }
    }
    return histogram;
}

// ---------------------------------------------------------------------------
// Equalizes one 8 bit channel.
//
// Mapping (the curve drawn by Task 4):
//     LUT[i] = round( (cdf(i) - cdfMin) * 255 / (N - cdfMin) )
// where cdfMin is the first non zero cumulative count. Using cdfMin keeps the
// darkest gray value at 0 instead of stretching a mostly black image.
//
// This is mathematically what cv::equalizeHist does - it is written out here
// because the assignment requires the CDF built in Task 4 to be used.
// ---------------------------------------------------------------------------
cv::Mat equalizeChannel(const cv::Mat& gray8)
{
    const Histogram histogram = computeHistogram(gray8);

    const long long total = static_cast<long long>(gray8.rows) * gray8.cols;

    // First non zero bin -> its cumulative count is cdfMin.
    long long cdfMin = 0;
    bool found = false;
    for (int value = 0; value < 256; ++value) {
        if (histogram[value] > 0) {
            cdfMin = histogram[value];
            found = true;
            break;
        }
    }

    // Uniform image: every pixel is the same value, nothing to stretch.
    if (!found || total <= cdfMin) {
        return gray8.clone();
    }

    cv::Mat lookup(1, 256, CV_8U);
    long long cumulative = 0;
    for (int value = 0; value < 256; ++value) {
        cumulative += histogram[value];
        const double scaled = static_cast<double>(cumulative - cdfMin) * 255.0
                              / static_cast<double>(total - cdfMin);
        const int rounded = static_cast<int>(std::lround(scaled));
        lookup.at<uchar>(0, value) = cv::saturate_cast<uchar>(rounded);
    }

    cv::Mat equalized;
    cv::LUT(gray8, lookup, equalized);
    return equalized;
}

} // namespace

// ---------------------------------------------------------------------------
// Returns the contrast-enhanced image.
//
//   * grayscale image -> the whole image is equalized
//   * colour image    -> converted to YCrCb, only the luminance (Y) is
//                       equalized and the colour channels are kept as they
//                       are (equalizing B, G and R independently would shift
//                       the hues).
// ---------------------------------------------------------------------------
cv::Mat Task5_EqualizationService::equalize(const cv::Mat& image)
{
    if (image.empty()) {
        return {};
    }

    if (image.channels() == 1) {
        const cv::Mat gray = toGray8(image);
        if (gray.empty()) {
            return {};
        }
        return equalizeChannel(gray);
    }

    cv::Mat color;
    if (image.channels() == 4) {
        cv::cvtColor(image, color, cv::COLOR_BGRA2BGR);
    } else {
        color = image;
    }

    cv::Mat ycrcb;
    cv::cvtColor(color, ycrcb, cv::COLOR_BGR2YCrCb);

    std::vector<cv::Mat> channels;
    cv::split(ycrcb, channels);

    // channels[0] is the luminance Y.
    if (channels[0].depth() != CV_8U) {
        cv::Mat gray;
        channels[0].convertTo(gray, CV_8U);
        channels[0] = equalizeChannel(gray);
    } else {
        channels[0] = equalizeChannel(channels[0]);
    }

    cv::merge(channels, ycrcb);

    cv::Mat result;
    cv::cvtColor(ycrcb, result, cv::COLOR_YCrCb2BGR);
    return result;
}
