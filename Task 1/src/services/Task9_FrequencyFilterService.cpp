#include "services/Task9_FrequencyFilterService.h"

#include <opencv2/core.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

// ============================================================================
// Task 9 - Frequency Domain Filtering
// High pass filter and low pass filter implemented in the frequency domain:
//   pad -> FFT (cv::dft) -> multiply by the Gaussian mask -> inverse FFT
//   (cv::idft) -> crop back to the original size.
// ============================================================================

namespace {

constexpr double kPi = 3.14159265358979323846;

// Everything that depends only on the image size and the cut-off. It is built
// once per call and shared by all colour channels.
struct FilterPlan
{
    // Border added around the image before the FFT (see makePlan).
    int top = 0;
    int bottom = 0;
    int left = 0;
    int right = 0;

    // Filter transfer function H(u,v) as a CV_32FC2 matrix (the same value in
    // the real and the imaginary channel, so it can multiply the complex
    // spectrum directly). It uses the order cv::dft produces (DC at [0,0]), so
    // no fftshift (quadrant swap) is needed.
    cv::Mat mask;
};

FilterPlan makePlan(int rows, int cols, double cutoffRadius, bool highPass)
{
    FilterPlan plan;

    const double d0 = std::max(cutoffRadius, 1e-3);
    const double referenceSize = static_cast<double>(std::max(rows, cols));

    // --- Border handling ------------------------------------------------------
    // The FFT filters "circularly": the image is treated as if it repeated
    // forever, so pixels near one border would be mixed with the opposite
    // border (and zero padding would add dark halos). To avoid that, the image
    // is extended by a mirrored border wider than the filter's reach before the
    // FFT, and cropped afterwards. A Gaussian of cutoff D0 equals a spatial
    // Gaussian with sigma = referenceSize / (2*pi*D0) pixels; 3 sigma covers it.
    // The border is then rounded up to a size cv::dft is fast for.
    const double sigma = referenceSize / (2.0 * kPi * d0);
    const double marginD = std::min(std::ceil(3.0 * sigma), referenceSize);
    const int margin = static_cast<int>(marginD);
    const int marginY = std::min(margin, rows - 1); // a mirrored border must stay inside the image
    const int marginX = std::min(margin, cols - 1);

    const int paddedRows = cv::getOptimalDFTSize(rows + 2 * marginY);
    const int paddedCols = cv::getOptimalDFTSize(cols + 2 * marginX);

    plan.top = marginY;
    plan.bottom = paddedRows - rows - marginY;
    plan.left = marginX;
    plan.right = paddedCols - cols - marginX;

    // --- Filter mask ---------------------------------------------------------
    // D^2 splits into a column term and a row term. A frequency index k in the
    // unshifted spectrum stands for the signed frequency k (k <= N/2) or k - N,
    // which is exactly its distance from DC - same result as shifting the
    // spectrum to the centre first, without having to swap quadrants.
    std::vector<double> colTerm(static_cast<std::size_t>(paddedCols));
    std::vector<double> rowTerm(static_cast<std::size_t>(paddedRows));
    for (int x = 0; x < paddedCols; ++x) {
        const int signedIndex = (x <= paddedCols / 2) ? x : x - paddedCols;
        const double u = signedIndex * referenceSize / paddedCols;
        colTerm[static_cast<std::size_t>(x)] = u * u;
    }
    for (int y = 0; y < paddedRows; ++y) {
        const int signedIndex = (y <= paddedRows / 2) ? y : y - paddedRows;
        const double v = signedIndex * referenceSize / paddedRows;
        rowTerm[static_cast<std::size_t>(y)] = v * v;
    }

    const double twoD0Squared = 2.0 * d0 * d0;
    plan.mask.create(paddedRows, paddedCols, CV_32FC2);
    for (int y = 0; y < paddedRows; ++y) {
        cv::Vec2f* row = plan.mask.ptr<cv::Vec2f>(y);
        for (int x = 0; x < paddedCols; ++x) {
            const double exponent = -(rowTerm[static_cast<std::size_t>(y)]
                                      + colTerm[static_cast<std::size_t>(x)]) / twoD0Squared;
            // low pass  : exp(-D^2 / 2 D0^2)
            // high pass : 1 - exp(...) = -expm1(...)  (accurate near DC)
            const float value = highPass ? static_cast<float>(-std::expm1(exponent))
                                         : static_cast<float>(std::exp(exponent));
            row[x] = cv::Vec2f(value, value);
        }
    }

    return plan;
}

// One single-channel CV_32F plane: pad -> FFT -> mask -> inverse FFT -> crop.
// Every intermediate result lives in its own matrix; `plane` is only read.
cv::Mat filterPlane(const cv::Mat& plane, const FilterPlan& plan)
{
    cv::Mat padded;
    cv::copyMakeBorder(plane, padded, plan.top, plan.bottom, plan.left, plan.right,
                       cv::BORDER_REFLECT_101);

    cv::Mat spectrum; // complex spectrum, DC at [0,0]
    cv::dft(padded, spectrum, cv::DFT_COMPLEX_OUTPUT);

    cv::Mat filteredSpectrum;
    cv::multiply(spectrum, plan.mask, filteredSpectrum);

    cv::Mat filtered; // back in the spatial domain (real, already divided by N)
    cv::idft(filteredSpectrum, filtered, cv::DFT_SCALE | cv::DFT_REAL_OUTPUT);

    return filtered(cv::Rect(plan.left, plan.top, plane.cols, plane.rows)).clone();
}

// Filters every colour channel of `image` and returns CV_32F (same channels).
cv::Mat filterToFloat(const cv::Mat& image, double cutoffRadius, bool highPass)
{
    if (image.empty()) {
        return cv::Mat();
    }

    cv::Mat work; // float COPY of the input - the caller's image stays untouched
    image.convertTo(work, CV_32F);

    std::vector<cv::Mat> channels;
    cv::split(work, channels);

    // A 4th channel is alpha (transparency): high pass would turn it into 0 and
    // make the image invisible, so only B, G, R are filtered.
    const int channelCount = static_cast<int>(channels.size());
    const int filteredCount = (channelCount == 4) ? 3 : channelCount;

    const FilterPlan plan = makePlan(image.rows, image.cols, cutoffRadius, highPass);
    for (int c = 0; c < filteredCount; ++c) {
        channels[static_cast<std::size_t>(c)] = filterPlane(channels[static_cast<std::size_t>(c)], plan);
    }

    cv::Mat result;
    cv::merge(channels, result);
    return result;
}

// Value below which `fraction` of all samples of the first `count` planes lie
// (at most ~1M samples are looked at, so large images stay fast).
double percentileOfPlanes(const std::vector<cv::Mat>& planes, int count, double fraction)
{
    std::size_t total = 0;
    for (int c = 0; c < count; ++c) {
        total += planes[static_cast<std::size_t>(c)].total();
    }
    if (total == 0) {
        return 0.0;
    }

    const std::size_t step = std::max<std::size_t>(1, total / 1000000);
    std::vector<float> samples;
    samples.reserve(total / step + static_cast<std::size_t>(count));

    for (int c = 0; c < count; ++c) {
        const cv::Mat plane = planes[static_cast<std::size_t>(c)].isContinuous()
                                  ? planes[static_cast<std::size_t>(c)]
                                  : planes[static_cast<std::size_t>(c)].clone();
        const float* data = plane.ptr<float>(0);
        const std::size_t n = plane.total();
        for (std::size_t i = 0; i < n; i += step) {
            samples.push_back(data[i]);
        }
    }

    const std::size_t k = static_cast<std::size_t>(fraction * static_cast<double>(samples.size() - 1));
    std::nth_element(samples.begin(), samples.begin() + static_cast<std::ptrdiff_t>(k), samples.end());
    return static_cast<double>(samples[k]);
}

} // namespace

// ----------------------------------------------------------------------------
// Low pass: keep the low frequencies -> smooth / blurred image.
// ----------------------------------------------------------------------------
cv::Mat Task9_FrequencyFilterService::lowPass(const cv::Mat& image, double cutoffRadius)
{
    if (image.empty()) {
        return cv::Mat();
    }

    cv::Mat result;
    // convertTo rounds and saturates, giving back the input's depth (8 / 16 bit ...).
    filterToFloat(image, cutoffRadius, false).convertTo(result, image.depth());
    return result;
}

// ----------------------------------------------------------------------------
// High pass: keep the high frequencies -> edges and fine detail.
// ----------------------------------------------------------------------------
cv::Mat Task9_FrequencyFilterService::highPass(const cv::Mat& image, double cutoffRadius)
{
    if (image.empty()) {
        return cv::Mat();
    }

    // Signed detail: positive on the bright side of an edge, negative on the dark side.
    const cv::Mat detail = filterToFloat(image, cutoffRadius, true);

    std::vector<cv::Mat> channels;
    cv::split(detail, channels);
    const int channelCount = static_cast<int>(channels.size());
    const int colourCount = (channelCount == 4) ? 3 : channelCount; // alpha stays as it is

    // Make it displayable: |detail| (an 8 bit image cannot hold negative values),
    // stretched so the strong details are bright. The 99.5th percentile is used
    // instead of the maximum so a few extreme pixels do not make everything dark.
    for (int c = 0; c < colourCount; ++c) {
        cv::Mat& plane = channels[static_cast<std::size_t>(c)];
        cv::absdiff(plane, cv::Scalar::all(0), plane);
    }

    const int depth = image.depth();
    if (depth == CV_8U || depth == CV_16U) {
        const double fullScale = (depth == CV_16U) ? 65535.0 : 255.0;
        const double oneGreyLevel = fullScale / 255.0; // never amplify below 1 grey level (noise)
        const double reference = std::max(percentileOfPlanes(channels, colourCount, 0.995), oneGreyLevel);
        const double gain = fullScale / reference;
        for (int c = 0; c < colourCount; ++c) {
            cv::Mat& plane = channels[static_cast<std::size_t>(c)];
            plane.convertTo(plane, CV_32F, gain);
        }
    }

    cv::Mat merged;
    cv::merge(channels, merged);

    cv::Mat result;
    merged.convertTo(result, depth); // saturates values above the full scale
    return result;
}

// ----------------------------------------------------------------------------
// Raw versions for Task 10 (CV_32F, signed, no clipping).
// ----------------------------------------------------------------------------
cv::Mat Task9_FrequencyFilterService::lowPassFloat(const cv::Mat& image, double cutoffRadius)
{
    return filterToFloat(image, cutoffRadius, false);
}

cv::Mat Task9_FrequencyFilterService::highPassFloat(const cv::Mat& image, double cutoffRadius)
{
    return filterToFloat(image, cutoffRadius, true);
}
