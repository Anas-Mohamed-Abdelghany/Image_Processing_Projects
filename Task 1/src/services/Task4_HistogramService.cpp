#include "services/Task4_HistogramService.h"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <array>
#include <string>

// ============================================================================
// Task 4 - Histograms
// ============================================================================

namespace {

constexpr int kBins         = 256;
constexpr int kCanvasWidth  = 560;
constexpr int kCanvasHeight = 400;

// Plot area (leaves room for the axes and the labels).
constexpr int kMarginLeft   = 56;
constexpr int kMarginTop    = 30;
constexpr int kMarginRight  = 16;
constexpr int kMarginBottom = 44;

// BGR colours.
const cv::Scalar kBackground(24, 24, 28);
const cv::Scalar kAxis(190, 190, 190);
const cv::Scalar kGrid(70, 70, 74);
const cv::Scalar kLabel(170, 170, 170);

using Histogram = std::array<int, kBins>;

// ---------------------------------------------------------------------------
// The statistics of this task are computed on a single channel: colour images
// are reduced to grayscale first (the controller passes the current image as
// it is).
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

// Counts how often every gray value (0..255) occurs. Done by hand instead of
// cv::calcHist so the numbers used for the CDF are unambiguous.
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
// Empty plot area: dark background + light axes + a small grid. Both returned
// images share this canvas so the controller can put them side by side.
// ---------------------------------------------------------------------------
cv::Mat makeCanvas(const char* title)
{
    cv::Mat canvas(kCanvasHeight, kCanvasWidth, CV_8UC3, kBackground);

    const int left   = kMarginLeft;
    const int top    = kMarginTop;
    const int bottom = kCanvasHeight - kMarginBottom;
    const int right  = kCanvasWidth - kMarginRight;

    // Horizontal grid lines.
    for (int i = 1; i <= 4; ++i) {
        const int y = top + (bottom - top) * i / 5;
        cv::line(canvas, cv::Point(left, y), cv::Point(right, y), kGrid, 1, cv::LINE_AA);
    }

    // Axes.
    cv::line(canvas, cv::Point(left, top), cv::Point(left, bottom), kAxis, 1, cv::LINE_AA);
    cv::line(canvas, cv::Point(left, bottom), cv::Point(right, bottom), kAxis, 1, cv::LINE_AA);

    // Labels.
    cv::putText(canvas, title, cv::Point(left, 20),
                cv::FONT_HERSHEY_SIMPLEX, 0.55, kAxis, 1, cv::LINE_AA);
    cv::putText(canvas, "0", cv::Point(left - 4, bottom + 16),
                cv::FONT_HERSHEY_SIMPLEX, 0.4, kLabel, 1, cv::LINE_AA);
    cv::putText(canvas, "255", cv::Point(right - 24, bottom + 16),
                cv::FONT_HERSHEY_SIMPLEX, 0.4, kLabel, 1, cv::LINE_AA);
    cv::putText(canvas, "gray value", cv::Point(left + (right - left) / 2 - 34, bottom + 32),
                cv::FONT_HERSHEY_SIMPLEX, 0.4, kLabel, 1, cv::LINE_AA);

    return canvas;
}

// Bar colour: a smooth blue -> cyan gradient over the gray values.
cv::Scalar barColour(int bin)
{
    const double t = static_cast<double>(bin) / (kBins - 1);
    return cv::Scalar(255.0 - 215.0 * t, // B
                      120.0 + 100.0 * t, // G
                       60.0 +  40.0 * t); // R
}

} // namespace

// ---------------------------------------------------------------------------
// Bar chart of the gray level distribution (0..255 bins).
// ---------------------------------------------------------------------------
cv::Mat Task4_HistogramService::drawHistogram(const cv::Mat& image)
{
    const cv::Mat gray = toGray8(image);
    if (gray.empty()) {
        return {};
    }

    const Histogram histogram = computeHistogram(gray);

    int maxCount = 0;
    for (int count : histogram) {
        if (count > maxCount) {
            maxCount = count;
        }
    }

    cv::Mat canvas = makeCanvas("Histogram");

    const int left   = kMarginLeft;
    const int top    = kMarginTop;
    const int bottom = kCanvasHeight - kMarginBottom;
    const int right  = kCanvasWidth - kMarginRight;
    const int plotWidth  = right - left;
    const int plotHeight = bottom - top;

    // Value labels for the grid lines (share of the tallest bin).
    for (int i = 1; i <= 4; ++i) {
        const int y = top + plotHeight * i / 5;
        const int value = maxCount * (5 - i) / 5;
        cv::putText(canvas, std::to_string(value), cv::Point(4, y + 4),
                    cv::FONT_HERSHEY_SIMPLEX, 0.4, kLabel, 1, cv::LINE_AA);
    }
    cv::putText(canvas, "count", cv::Point(6, top - 8),
                cv::FONT_HERSHEY_SIMPLEX, 0.4, kLabel, 1, cv::LINE_AA);

    if (maxCount > 0) {
        for (int bin = 0; bin < kBins; ++bin) {
            const int x0 = left + bin * plotWidth / kBins;
            const int x1 = left + (bin + 1) * plotWidth / kBins;
            const int height = static_cast<int>(
                static_cast<double>(histogram[bin]) * plotHeight / maxCount);

            if (height <= 0) {
                continue;
            }
            cv::rectangle(canvas,
                          cv::Point(x0, bottom - height),
                          cv::Point(std::max(x1 - 1, x0 + 1), bottom),
                          barColour(bin), cv::FILLED);
        }
    }

    return canvas;
}

// ---------------------------------------------------------------------------
// Cumulative distribution function: cdf(i) = (#pixels <= i) / #totalPixels.
// This is exactly the mapping curve that Task 5 uses for equalization.
// ---------------------------------------------------------------------------
cv::Mat Task4_HistogramService::drawCumulativeCurve(const cv::Mat& image)
{
    const cv::Mat gray = toGray8(image);
    if (gray.empty()) {
        return {};
    }

    const Histogram histogram = computeHistogram(gray);

    const double total = static_cast<double>(gray.rows) * static_cast<double>(gray.cols);
    if (total <= 0.0) {
        return makeCanvas("Cumulative distribution (CDF)");
    }

    std::vector<cv::Point> points;
    points.reserve(kBins);

    long long cumulative = 0;
    const int left   = kMarginLeft;
    const int top    = kMarginTop;
    const int bottom = kCanvasHeight - kMarginBottom;
    const int right  = kCanvasWidth - kMarginRight;
    const int plotWidth  = right - left;
    const int plotHeight = bottom - top;

    for (int bin = 0; bin < kBins; ++bin) {
        cumulative += histogram[bin];
        const int x = left + bin * plotWidth / (kBins - 1);
        const int y = bottom - static_cast<int>((cumulative / total) * plotHeight);
        points.emplace_back(x, y);
    }

    cv::Mat canvas = makeCanvas("Cumulative distribution (CDF)");

    // Percentage labels next to the grid lines.
    for (int i = 1; i <= 4; ++i) {
        const int y = top + plotHeight * i / 5;
        const int percent = (5 - i) * 20;
        cv::putText(canvas, std::to_string(percent) + "%", cv::Point(6, y + 4),
                    cv::FONT_HERSHEY_SIMPLEX, 0.4, kLabel, 1, cv::LINE_AA);
    }
    cv::putText(canvas, "P(x)", cv::Point(6, top - 8),
                cv::FONT_HERSHEY_SIMPLEX, 0.4, kLabel, 1, cv::LINE_AA);

    cv::polylines(canvas, points, false /* not closed */, cv::Scalar(80, 220, 80), 2, cv::LINE_AA);

    // End point marker: the curve always ends at 100%.
    if (!points.empty()) {
        cv::circle(canvas, points.back(), 3, cv::Scalar(80, 220, 80), cv::FILLED, cv::LINE_AA);
    }

    return canvas;
}
